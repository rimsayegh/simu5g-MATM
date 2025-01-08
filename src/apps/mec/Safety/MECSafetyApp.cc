//
//                  Simu5G
//
// Authors: Giovanni Nardini, Giovanni Stea, Antonio Virdis (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

//#define FMT_HEADER_ONLY
#include "apps/mec/Safety/MECSafetyApp.h"

#include "apps/mec/DeviceApp/DeviceAppMessages/DeviceAppPacket_Types.h"
#include "apps/mec/Safety/packets/SafetyPacket_Types.h"

#include "inet/common/TimeTag_m.h"
#include "inet/common/packet/Packet_m.h"

#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/transportlayer/common/L4PortTag_m.h"

#include "nodes/mec/utils/httpUtils/httpUtils.h"
#include "nodes/mec/utils/httpUtils/json.hpp"
#include "nodes/mec/MECPlatform/MECServices/packets/HttpResponseMessage/HttpResponseMessage.h"

#include <random>

#include <fstream>



//#include "spdlog/spdlog.h"  // logging library
//#include "spdlog/sinks/basic_file_sink.h"
#include <ctime>
//#include <fmt/format.h>

#include <iostream>
#include <iomanip>
#include <random>
#include <string>
#include <map>
#include <iostream>
#include <sys/stat.h>
#include "CommonFunctions.h"


Define_Module(MECSafetyApp);


std::ofstream SAMFile;
std::ofstream DelayFile;

using namespace inet;
using namespace omnetpp;


simsignal_t MECSafetyApp::mecapp_ul_delaySignal =registerSignal("mecapp_ul_delaySignal") ;
simsignal_t MECSafetyApp:: mecapp_pk_rcv_sizeSignal =registerSignal("mecapp_pk_rcv_sizeSignal") ;


MECSafetyApp::MECSafetyApp(): MecAppBase()
{
    circle = nullptr; // circle danger zone
    pingPongprocessMessage_ = nullptr;

}
MECSafetyApp::~MECSafetyApp()
{
    if(circle != nullptr)
    {
        if(getSimulation()->getSystemModule()->getCanvas()->findFigure(circle) != -1)
            getSimulation()->getSystemModule()->getCanvas()->removeFigure(circle);
        delete circle;
    }

    cancelAndDelete(pingPongprocessMessage_);
}

int get_packet_dimension() {
    static std::default_random_engine generator;
    static std::normal_distribution<double> distribution(1024.0,100.0);

    return int(distribution(generator));
}

double MECSafetyApp::computationTime(int port)
{
    double ins;
    ins= exponentialGenerator(1.0 / (500*pow(10, 6)));

    ins = (ins<0)? 0:ins;
    double processingTime_ = vim->calculateProcessingTime(mecAppId, ins);

    return processingTime_;
}


double MECSafetyApp::scheduleNextMsg(cMessage* msg)
{
    // determine its source address/port

    if (!strcmp(msg->getName(), "DataPacket"))
    {

//        try {

        auto pk = check_and_cast<Packet *>(msg);
        ueAppAddress = pk->getTag<L3AddressInd>()->getSrcAddress();
        ueAppPort = pk->getTag<L4PortInd>()->getSrcPort();
        auto mecPk = pk->peekAtFront<DataPacket>();
        auto pubPk = dynamicPtrCast<const DataPacket>(mecPk);


        double processingTime {computationTime(ueAppPort)} ;

        // emit statistics UL Delay

        simtime_t Delay = pk->getArrivalTime() - pk->getCreationTime();
        emit(ULPacketDelay_, Delay);
        emit(ULpacketRcvdMsg_, (long)1);
        ULnrReceived++;
        ULdelaySum+=Delay;

        EV <<"] ULPacketDelay[" << Delay << "]" << endl;

        // emit statistics Queue Time
        simtime_t QDelay = simTime() - msg->getArrivalTime();
        emit(QueueingMECDelay_, QDelay);
        QnrReceived++;
        QdelaySum+=QDelay;

        EV <<"] QueueingMECDelay[" << QDelay << "]" << endl;

        // emit statistics Processing Time
        emit(processingMECDelay_, processingTime);
        PnrReceived++;
        PdelaySum+=processingTime;

        EV <<"]processingMECDelay[" << processingTime << "]" << endl;


        DelayFile.open("ULdelays.txt", std::ios::app);  // Open file for appending

        if (DelayFile.is_open()) {
            DelayFile << "MecAppID: "<< mecAppId  <<" , "<< " ULPacketDelay: "<< Delay << " , "<< " QueueingDelay: "<< QDelay <<" , "<< " ProcessingDelay: "<< processingTime << " , "<< " Calculated at time: "<< simTime() << std::endl;
            DelayFile.close();
        }

        return processingTime;


            }
    else {
        double processingTime = vim->calculateProcessingTime(mecAppId, 20);// int(uniform(200, 400))); this cause machines to invert orders!
        return processingTime;
    }

}


void MECSafetyApp::handleMessage(cMessage *msg)
{
    /*SAMFile.open("mecapp.txt", std::ios::app);  // Open file for appending

    if (SAMFile.is_open()) {
        SAMFile << "Received message with name: "<< msg->getName() << std::endl;
        SAMFile.close();
    }
    if (auto *httpMsg = dynamic_cast<HttpRequestMessage *>(msg)) {
        SAMFile.open("mecapp.txt", std::ios::app);  // Open file for appending

        if (SAMFile.is_open()) {
            SAMFile << "Message type: " << httpMsg->getType() << " with content: " << httpMsg->getBody() << std::endl;
            SAMFile.close();
        }
    }*/

    if (msg->isSelfMessage())
    {


        if(strcmp(msg->getName(), "processedMessage") == 0)
        {
            handleProcessedMessage((cMessage*)packetQueue_.pop());
            //delete packetQueue_.pop();
            if(!packetQueue_.isEmpty())
            {
                double processingTime = scheduleNextMsg((cMessage *)packetQueue_.front());
                EV <<"MecAppBase::scheduleNextMsg() - next msg is processed in " << processingTime << "s" << endl;
                scheduleAt(simTime() + processingTime, processMessage_);
            }
            else
            {
                EV << "MecAppBase::handleMessage - no more messages are present in the queue" << endl;
            }
        }
        else if(strcmp(msg->getName(), "processedHttpMsg") == 0)
        {
            EV << "MecAppBase::handleMessage(): processedHttpMsg " << endl;
            ProcessingTimeMessage * procMsg = check_and_cast<ProcessingTimeMessage*>(msg);
            int connId = procMsg->getSocketId();
            TcpSocket* sock = (TcpSocket*)sockets_.getSocketById(connId);
            if(sock != nullptr)
            {
                HttpMessageStatus* msgStatus = (HttpMessageStatus *)sock->getUserData();
                handleHttpMessage(connId);
                SAMFile.open("mecapp.txt", std::ios::app);  // Open file for appending
                delete msgStatus->httpMessageQueue.pop();

                if (SAMFile.is_open()) {
                   SAMFile << "Teeeesssssttttttt at: "<< simTime()<< std::endl;
                   SAMFile.close();
                }

                if(!msgStatus->httpMessageQueue.isEmpty())
                {
                    EV << "MecAppBase::handleMessage(): processedHttpMsg - the httpMessageQueue is not empty, schedule next HTTP message" << endl;
                    double time = vim->calculateProcessingTime(mecAppId, 150);
                    scheduleAt(simTime()+time, msg);
                }
            }
        }
        else
        {
            handleSelfMessage(msg);

        }
    }
    else
    {
        /*if (!strcmp(msg->getName(), "DataPacket")){
            try{
            auto pk = check_and_cast<Packet *>(msg);
            ueAppAddress = pk->getTag<L3AddressInd>()->getSrcAddress();
            ueAppPort = pk->getTag<L4PortInd>()->getSrcPort();
            auto mecPk = pk->peekAtFront<DataPacket>();
            auto pubPk = dynamicPtrCast<const DataPacket>(mecPk);

            }
            catch (const cRuntimeError& error){
        delete msg;
            }
            }*/
        if(!processMessage_->isScheduled() && packetQueue_.isEmpty())
        {
            packetQueue_.insert(msg);
            double processingTime;
            if(strcmp(msg->getFullName(), "data") == 0)
                processingTime = MecAppBase::scheduleNextMsg(msg);
            else
               processingTime = scheduleNextMsg(msg);
            EV <<"MecAppBase::scheduleNextMsg() - next msg is processed in " << processingTime << "s" << endl;
            scheduleAt(simTime() + processingTime, processMessage_);

        }
        else if (processMessage_->isScheduled() && !packetQueue_.isEmpty())
        {
            packetQueue_.insert(msg);
        }
        else
        {
            throw cRuntimeError("MecAppBase::handleMessage - This situation is not possible");
        }
    }
}



void MECSafetyApp::initialize(int stage)
{
    MecAppBase::initialize(stage);

    // avoid multiple initializations
    if (stage!=inet::INITSTAGE_APPLICATION_LAYER)
        return;
    if (stage == INITSTAGE_LOCAL)
           {

           }
    nbSentMsg = 0;
    //retrieving parameters
    size_ = par("packetSize");

    // set Udp Socket
    ueSocket.setOutputGate(gate("socketOut"));

    localUePort = par("localUePort");
    ueSocket.bind(localUePort);
    //maxCpuSpeed_ = par("maxCpuSpeed");
    //maxCpuSpeed_ = 300000;
    ULPacketDelay_ = registerSignal("ULPacketDelay");
    ULpacketRcvdMsg_ = registerSignal("ULpacketRcvdMsg");

    QueueingMECDelay_ = registerSignal("QueueingMECDelay");
    processingMECDelay_ = registerSignal("processingMECDelay");


    QdelaySum = 0;
    ULdelaySum = 0;
    PdelaySum = 0;

    ULnrReceived = 0;
    QnrReceived =0;
    PnrReceived =0;

    //testing
    EV << "MECSafetyApp::initialize - Mec application "<< getClassName() << " with mecAppId["<< mecAppId << "] has started!" << endl;

    mp1Socket_ = addNewSocket();

    circle = new cOvalFigure("circle");


    //Radius_ = par("radius");

    // connect with the service registry
    cMessage *msg = new cMessage("connectMp1");
    scheduleAt(simTime() + 0, msg);

}


void MECSafetyApp::finish(){
    MecAppBase::finish();
    EV << "MECSafetyApp::finish()" << endl;

    if(gate("socketOut")->isConnected()){

    }
}

void MECSafetyApp::handleUeMessage(omnetpp::cMessage *msg)
{
    // determine its source address/port
    auto pk = check_and_cast<Packet *>(msg);
    ueAppAddress = pk->getTag<L3AddressInd>()->getSrcAddress();
    ueAppPort = pk->getTag<L4PortInd>()->getSrcPort();

    auto mecPk = pk->peekAtFront<DataPacket>();

    if(strcmp(mecPk->getType(), START_WARNING) == 0)
    {
        /*
         * Read center and radius from message
         */
        EV << "MECSafetyApp::handleUeMessage - WarningStartPacket arrived" << endl;
        auto warnPk = dynamicPtrCast<const DataPacket>(mecPk);
        if(warnPk == nullptr)
            throw cRuntimeError("MECSafetyApp::handleUeMessage - WarningStartPacket is null");

        centerPositionX = warnPk->getCenterPositionX();
        centerPositionY = warnPk->getCenterPositionY();
        radius = warnPk->getRadius();

        omnetpp::simtime_t A= warnPk->getPayloadTimestamp();

        SAMFile.open("Interruption.txt", std::ios::app);  // Open file for appending

        if (SAMFile.is_open()) {
            SAMFile << "Interruption Time : "<< simTime()-A << std::endl;
            SAMFile.close();
        }
        // emit statistics
//        simtime_t startdelay = simTime() - warnPk->getPayloadTimestamp();
//        emit(startAppDelay_, startdelay);
//        startnrReceived++;
//        startdelaySum+=startdelay;
//
//        EV <<"] startAppDelay[" << startdelay << "]" << endl;

        if(par("logger").boolValue())
        {
            ofstream myfile;
            myfile.open ("example.txt", ios::app);
            if(myfile.is_open())
            {
                myfile <<"["<< NOW << "] MECSafetyApp - Received message from UE, connecting to the Location Service\n";
                myfile.close();
            }
        }

        cModule* mec = this->getParentModule();
        SAMFile.open("migration.txt", std::ios::app);  // Open file for appending

        if (SAMFile.is_open()) {
            SAMFile << "MECSafetyApp - Received message from UEs:"<< mecAppId << "and the mecHost is: " << mec << std::endl;
            SAMFile << "MECSafetyApp - Received message from UEs at: "<< simTime() << std::endl;
            SAMFile.close();
        }

    cMessage *m = new cMessage("connectService");
        scheduleAt(simTime()+0.005, m);


        /*SAMFile.open("mecapp.txt", std::ios::app);  // Open file for appending

        if (SAMFile.is_open()) {
           SAMFile << "MECSafetyApp - Received message from UEs: "<< mecAppId << " and the mecHost is: " << mec << std::endl;
           SAMFile << "The centerX: "<< centerPositionX << std::endl;
           SAMFile.close();
        }*/
    }
    else if (strcmp(mecPk->getType(), STOP_WARNING) == 0)
    {
        sendDeleteSubscription();
        //////////////////////////////////////I shoud add a function to stop the sending tasks from vehicles)/////////////////////////////////////

        SAMFile.open("mecapp.txt", std::ios::app);  // Open file for appending

        if (SAMFile.is_open()) {
            SAMFile << "MECSafetyApp - Send Delete Subscription at: " << simTime() << std::endl;
            SAMFile.close();
        }

    }
    else if(!strcmp(msg->getName(), "DataPacket")) {

               //myLogger->info(fmt::format("handleMessage: Received Ping packet from UE - {}s ",simTime().str()));
               // determine its source address/port




                     emit( mecapp_pk_rcv_sizeSignal,   pk->getByteLength());
                     emit( mecapp_ul_delaySignal,  simTime() - pk->getCreationTime() );



                     sendDataPacket(msg);

                     cModule* Mec = this->getParentModule();

                     SAMFile.open("mecapp.txt", std::ios::app);  // Open file for appending

                     if (SAMFile.is_open()) {
                         SAMFile << "MECSafetyApp - Send Data Packet to the vehicle from: "<< Mec<< std::endl;
                         SAMFile.close();
                     }




                //myLogger->info(fmt::format("handleMessage: Processing time should be - {}s",computationTime(uniform(200,300))));


               //delete msg;
               return;
           }

    else
    {
        throw cRuntimeError("MECSafetyApp::handleUeMessage - packet not recognized");
    }
}



void MECSafetyApp::sendDataPacket(cMessage *msg) {

    auto pk = check_and_cast<Packet *>(msg);
    ueAppAddress = pk->getTag<L3AddressInd>()->getSrcAddress();
    ueAppPort = pk->getTag<L4PortInd>()->getSrcPort();

    auto mecPk = pk->peekAtFront<DataPacket>();

    auto pubPk = dynamicPtrCast<const DataPacket>(mecPk);
    if (pubPk == nullptr)
        throw cRuntimeError("MECSafetyApp::handleUeMessage - DataPacket is null");

    auto pkt_ = inet::makeShared<DataPacket>();

    pkt_->setIDframe(pubPk->getIDframe());
    pkt_->setType(ANS_PINGPONG);
    pkt_->setdata("pongData");
   //pkt_->setChunkLength(inet::B(get_packet_dimension()));

    int payload_; //default

    payload_ = 313;

    pkt_->setChunkLength(inet::B(payload_));

    pkt_->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
    inet::Packet* pkt_to_send = new inet::Packet("DataPacket");
    pkt_to_send->insertAtBack(pkt_);
    //myLogger->info(fmt::format("222222Sending to ueAppAddress {}///ueAppPort {}///",ueAppAddress.str(),ueAppPort));

    ueSocket.sendTo(pkt_to_send, ueAppAddress, ueAppPort);

    SAMFile.open("mecapp.txt", std::ios::app);  // Open file for appending

     if (SAMFile.is_open()) {
         SAMFile << "MECSafetyApp - sending to address: "<< ueAppAddress << " port: "<< ueAppPort << std::endl;
         SAMFile.close();
     }

}

void MECSafetyApp::modifySubscription(){
    std::string body = "{  \"circleNotificationSubscription\": {"
                       "\"callbackReference\" : {"
                        "\"callbackData\":\"1234\","
                        "\"notifyURL\":\"example.com/notification/1234\"},"
                       "\"checkImmediate\": \"false\","
                        "\"address\": \"" + ueAppAddress.str()+ "\","
                        "\"clientCorrelator\": \"null\","
                        "\"enteringLeavingCriteria\": \"Leaving\","
                        "\"frequency\": 5,"
                        "\"radius\": " + std::to_string(radius) + ","
                        "\"trackingAccuracy\": 10,"
                        "\"latitude\": " + std::to_string(centerPositionX) + ","           // as x
                        "\"longitude\": " + std::to_string(centerPositionY) + ","        // as y
                        "\"mecAppId\": \"" + std::to_string(mecAppId) + "\""
                        "}"
                        "}\r\n";
    std::string uri = "/example/location/v2/subscriptions/area/circle/" + subId;
    std::string host = serviceSocket_->getRemoteAddress().str()+":"+std::to_string(serviceSocket_->getRemotePort());
    Http::sendPutRequest(serviceSocket_, body.c_str(), host.c_str(), uri.c_str());
}

void MECSafetyApp::sendSubscription()
{
    std::string body = "{  \"circleNotificationSubscription\": {"
                           "\"callbackReference\" : {"
                            "\"callbackData\":\"1234\","
                            "\"notifyURL\":\"example.com/notification/1234\"},"
                           "\"checkImmediate\": \"false\","
                            "\"address\": \"" + ueAppAddress.str()+ "\","
                            "\"clientCorrelator\": \"null\","
                            "\"enteringLeavingCriteria\": \"Entering\","
                            "\"frequency\": 5,"
                            "\"radius\": " + std::to_string(radius) + ","
                            "\"trackingAccuracy\": 10,"
                            "\"latitude\": " + std::to_string(centerPositionX) + ","           // as x
                            "\"longitude\": " + std::to_string(centerPositionY) + ","        // as y
                            "\"mecAppId\": \"" + std::to_string(mecAppId) + "\""
                            "}"
                            "}\r\n";
    std::string uri = "/example/location/v2/subscriptions/area/circle";
    std::string host = serviceSocket_->getRemoteAddress().str()+":"+std::to_string(serviceSocket_->getRemotePort());


    //Test //L2S-ESME
     /*SAMFile.open("mecapp.txt", std::ios::app);  // Open file for appending

     if (SAMFile.is_open()) {
         SAMFile << "Test:"<< mecAppId << std::endl;
         SAMFile.close();
     }*/
    if(par("logger").boolValue())
    {
        ofstream myfile;
        myfile.open ("example.txt", ios::app);
        if(myfile.is_open())
        {
            myfile <<"["<< NOW << "] MEWarningAlertApp - Sent POST circleNotificationSubscription the Location Service \n";
            myfile.close();
        }
    }

    Http::sendPostRequest(serviceSocket_, body.c_str(), host.c_str(), uri.c_str());

    /*SAMFile.open("mecapp.txt", std::ios::app);  // Open file for appending

    if (SAMFile.is_open()) {
         SAMFile << "Send Post Request to the service"<< std::endl;
         SAMFile.close();
    }*/
}

void MECSafetyApp::sendDeleteSubscription()
{
    std::string uri = "/example/location/v2/subscriptions/area/circle/" + subId;
    std::string host = serviceSocket_->getRemoteAddress().str()+":"+std::to_string(serviceSocket_->getRemotePort());
    Http::sendDeleteRequest(serviceSocket_, host.c_str(), uri.c_str());
}

void MECSafetyApp::established(int connId)
{
    /*SAMFile.open("mecapp.txt", std::ios::app);  // Open file for appending

    if (SAMFile.is_open()) {
        SAMFile << " established "<< std::endl;
        SAMFile.close();
    }*/
    if(connId == mp1Socket_->getSocketId())
    {
        EV << "MECSafetyApp::established - Mp1Socket"<< endl;
        // get endPoint of the required service
        const char *uri = "/example/mec_service_mgmt/v1/services?ser_name=LocationService";
        std::string host = mp1Socket_->getRemoteAddress().str()+":"+std::to_string(mp1Socket_->getRemotePort());

        Http::sendGetRequest(mp1Socket_, host.c_str(), uri);
        return;
    }
    else if (connId == serviceSocket_->getSocketId())
    {
        EV << "MECSafetyApp::established - serviceSocket"<< endl;
        // the connectService message is scheduled after a start mec app from the UE app, so I can
        // response to her here, once the socket is established
        auto ack = inet::makeShared<SafetyAppPacket>();
        ack->setType(START_ACK);
        ack->setChunkLength(inet::B(2));
        inet::Packet* packet = new inet::Packet("WarningAlertPacketInfo");
        packet->insertAtBack(ack);
        ueSocket.sendTo(packet, ueAppAddress, ueAppPort);
        sendSubscription();

        /*SAMFile.open("mecapp.txt", std::ios::app);  // Open file for appending

        if (SAMFile.is_open()) {
            SAMFile << "MECSafetyApp::established - serviceSocket " << std::endl;
            SAMFile.close();
        }*/
        return;
    }
    else
    {
        throw cRuntimeError("MecAppBase::socketEstablished - Socket %d not recognized", connId);
    }
}


void MECSafetyApp::handleHttpMessage(int connId)
{
    SAMFile.open("mecapp.txt", std::ios::app);  // Open file for appending

    if (SAMFile.is_open()) {
            SAMFile << " In handleHttpMessage "<< std::endl;
            SAMFile.close();
    }

    //
    if (mp1Socket_ != nullptr && connId == mp1Socket_->getSocketId()) {
        handleMp1Message(connId);

        /*SAMFile.open("mecapp.txt", std::ios::app);  // Open file for appending

        if (SAMFile.is_open()) {
            SAMFile << "Handle Mp1 Message " << std::endl;
            SAMFile.close();
        }*/
    }
    else
    {
        handleServiceMessage(connId);

        /*SAMFile.open("mecapp.txt", std::ios::app);  // Open file for appending

        if (SAMFile.is_open()) {
            SAMFile << "Handle Service Message " << std::endl;
            SAMFile.close();
        }*/
    }
}

void MECSafetyApp::handleMp1Message(int connId)
{
    // for now I only have just one Service Registry
    HttpMessageStatus *msgStatus = (HttpMessageStatus*) mp1Socket_->getUserData();
    mp1HttpMessage = (HttpBaseMessage*) msgStatus->httpMessageQueue.front();
    EV << "MECSafetyApp::handleMp1Message - payload: " << mp1HttpMessage->getBody() << endl;

    SAMFile.open("mecapp.txt", std::ios::app);  // Open file for appending

    if (SAMFile.is_open()) {
        SAMFile << "The getUserData: "<<  mp1Socket_->getUserData() << std::endl;
        SAMFile.close();
    }

    try
    {
        nlohmann::json jsonBody = nlohmann::json::parse(mp1HttpMessage->getBody()); // get the JSON structure
        if(!jsonBody.empty())
        {
            jsonBody = jsonBody[0];
            std::string serName = jsonBody["serName"];
            if(serName.compare("LocationService") == 0)
            {
                if(jsonBody.contains("transportInfo"))
                {
                    nlohmann::json endPoint = jsonBody["transportInfo"]["endPoint"]["addresses"];
                    EV << "address: " << endPoint["host"] << " port: " <<  endPoint["port"] << endl;
                    std::string address = endPoint["host"];
                    serviceAddress = L3AddressResolver().resolve(address.c_str());
                    servicePort = endPoint["port"];
                    serviceSocket_ = addNewSocket();

                    /*SAMFile.open("mecapp.txt", std::ios::app);  // Open file for appending

                    if (SAMFile.is_open()) {
                        SAMFile << "The app connected to the service in function handleMp1Message, service port: "<< servicePort << std::endl;
                        SAMFile.close();
                    }*/
                }
            }
            else
            {
                EV << "MECSafetyApp::handleMp1Message - LocationService not found"<< endl;
                serviceAddress = L3Address();
            }
        }

    }
    catch(nlohmann::detail::parse_error e)
    {
        EV <<  e.what() << std::endl;
        // body is not correctly formatted in JSON, manage it
        return;
    }

}

void MECSafetyApp::handleServiceMessage(int connId)
{
    HttpMessageStatus *msgStatus = (HttpMessageStatus*) serviceSocket_->getUserData();
    serviceHttpMessage = (HttpBaseMessage*) msgStatus->httpMessageQueue.front();


    /*SAMFile.open("mecapp.txt", std::ios::app);  // Open file for appending

    if (SAMFile.is_open()) {
        SAMFile << "handleServiceMessage "<< std::endl;
        SAMFile.close();
    }

    SAMFile.open("mecapp.txt", std::ios::app);

    if (SAMFile.is_open()) {
        SAMFile << "serviceHttpMessage type: " << serviceHttpMessage->getType() << std::endl;
        SAMFile.close();
    }*/

    if(serviceHttpMessage->getType() == REQUEST)
    {
        SAMFile.open("mecapp.txt", std::ios::app);  // Open file for appending

        if (SAMFile.is_open()) {
            SAMFile << "Request"<< std::endl;
            SAMFile.close();
        }

        Http::send204Response(serviceSocket_); // send back 204 no content
        nlohmann::json jsonBody;
        EV << "MECSafetyApp::handleTcpMsg - REQUEST " << serviceHttpMessage->getBody()<< endl;
        try
        {
           jsonBody = nlohmann::json::parse(serviceHttpMessage->getBody()); // get the JSON structure

           /*SAMFile.open("mecapp.txt", std::ios::app);  // Open file for appending

           if (SAMFile.is_open()) {
               SAMFile << "Get the JSON structure after request"<< std::endl;
               SAMFile.close();
           }*/
        }
        catch(nlohmann::detail::parse_error e)
        {
           std::cout  <<  e.what() << std::endl;
           // body is not correctly formatted in JSON, manage it
           /*SAMFile.open("mecapp.txt", std::ios::app);  // Open file for appending

          if (SAMFile.is_open()) {
              SAMFile << "body is not correctly formatted in JSON, manage it"<< std::endl;
              SAMFile.close();
          }*/
           return;
        }

        if(jsonBody.contains("subscriptionNotification"))
        {
            if(jsonBody["subscriptionNotification"].contains("enteringLeavingCriteria"))
            {
                nlohmann::json criteria = jsonBody["subscriptionNotification"]["enteringLeavingCriteria"] ;
                auto alert = inet::makeShared<SafetyPacket>();
                alert->setType(WARNING_ALERT);

                if(criteria == "Entering")
                {
                    EV << "MECSafetyApp::handleTcpMsg - Ue is Entered in the danger zone "<< endl;
                    alert->setDanger(true);
                    alert->setPayloadTimestamp(simTime());

                    if(par("logger").boolValue())
                    {
                        ofstream myfile;
                        myfile.open ("example.txt", ios::app);
                        if(myfile.is_open())
                        {
                            myfile <<"["<< NOW << "] MEWarningAlertApp - Received circleNotificationSubscription notification from Location Service. UE's entered the red zone! \n";
                            myfile.close();
                        }
                    }

                    // send subscription for leaving..
                    modifySubscription();

                    SAMFile.open("mecapp.txt", std::ios::app);  // Open file for appending

                    if (SAMFile.is_open()) {
                        SAMFile << " modify subscription because it is entering the area "<< std::endl;
                        SAMFile.close();
                    }


                }
                else if (criteria == "Leaving")
                {
                    EV << "MECSafetyApp::handleTcpMsg - Ue left from the danger zone "<< endl;
                    alert->setDanger(false);
                    if(par("logger").boolValue())
                    {
                        ofstream myfile;
                        myfile.open ("example.txt", ios::app);
                        if(myfile.is_open())
                        {
                            myfile <<"["<< NOW << "] MEWarningAlertApp - Received circleNotificationSubscription notification from Location Service. UE's exited the red zone! \n";
                            myfile.close();
                        }
                    }
                    sendDeleteSubscription();

                    SAMFile.open("mecapp.txt", std::ios::app);  // Open file for appending

                    if (SAMFile.is_open()) {
                        SAMFile << " SendDeleteSubscription because it is leaving the area"<< std::endl;
                        SAMFile.close();
                    }
                }

                alert->setPositionX(jsonBody["subscriptionNotification"]["terminalLocationList"]["currentLocation"]["x"]);
                alert->setPositionY(jsonBody["subscriptionNotification"]["terminalLocationList"]["currentLocation"]["y"]);
                alert->setChunkLength(inet::B(20));
                alert->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());

                inet::Packet* packet = new inet::Packet("WarningAlertPacketInfo");
                packet->insertAtBack(alert);
                ueSocket.sendTo(packet, ueAppAddress, ueAppPort);

            }
        }
    }
    else if(serviceHttpMessage->getType() == RESPONSE)
    {
        /*SAMFile.open("mecapp.txt", std::ios::app);  // Open file for appending

        if (SAMFile.is_open()) {
            SAMFile << "Response"<< std::endl;
            SAMFile.close();
        }*/
        HttpResponseMessage *rspMsg = dynamic_cast<HttpResponseMessage*>(serviceHttpMessage);

        if(rspMsg->getCode() == 204) // in response to a DELETE
        {
            EV << "MECSafetyApp::handleTcpMsg - response 204, removing circle" << rspMsg->getBody()<< endl;
             serviceSocket_->close();
             getSimulation()->getSystemModule()->getCanvas()->removeFigure(circle);

        }
        else if(rspMsg->getCode() == 201) // in response to a POST
        {
            /*SAMFile.open("mecapp.txt", std::ios::app);  // Open file for appending

            if (SAMFile.is_open()) {
                SAMFile << " response to a post code = 201 "<< std::endl;
                SAMFile.close();
            }*/
            nlohmann::json jsonBody;
            EV << "MECSafetyApp::handleTcpMsg - response 201 " << rspMsg->getBody()<< endl;
            try
            {
               jsonBody = nlohmann::json::parse(rspMsg->getBody()); // get the JSON structure
            }
            catch(nlohmann::detail::parse_error e)
            {
               EV <<  e.what() << endl;
               // body is not correctly formatted in JSON, manage it
               return;
            }
            std::string resourceUri = jsonBody["circleNotificationSubscription"]["resourceURL"];
            std::size_t lastPart = resourceUri.find_last_of("/");
            if(lastPart == std::string::npos)
            {
                EV << "1" << endl;
                //Teesssstttt
                /*SAMFile.open("mecapp.txt", std::ios::app);  // Open file for appending

                if (SAMFile.is_open()) {
                    SAMFile << " 1 "<< std::endl;
                    SAMFile.close();
                }*/
                //Teeesssttt

                return;
            }
            // find_last_of does not take in to account if the uri has a last /
            // in this case subscriptionType would be empty and the baseUri == uri
            // by the way the next if statement solve this problem
            std::string baseUri = resourceUri.substr(0,lastPart);
            //save the id
            subId = resourceUri.substr(lastPart+1);
            EV << "subId: " << subId << endl;

            circle = new cOvalFigure("circle");
            circle->setBounds(cFigure::Rectangle(centerPositionX - radius, centerPositionY - radius,radius*2,radius*2));
            circle->setLineWidth(2);
            circle->setLineColor(cFigure::RED);
            getSimulation()->getSystemModule()->getCanvas()->addFigure(circle);

            /*SAMFile.open("mecapp.txt", std::ios::app);  // Open file for appending

            if (SAMFile.is_open()) {
                SAMFile << " Add Figure (circle) "<< std::endl;
                SAMFile.close();
            }*/

        }
    }

}

void MECSafetyApp::handleSelfMessage(cMessage *msg)
{
    if(strcmp(msg->getName(), "connectMp1") == 0)
    {
        EV << "MecAppBase::handleMessage- " << msg->getName() << endl;
        connect(mp1Socket_, mp1Address, mp1Port);
        //Connect to the service ????? maybe add delay defaut 0.05
        //cMessage *m = new cMessage("connectService");
        //scheduleAt(simTime()+1, m);

        /*SAMFile.open("mecapp.txt", std::ios::app);  // Open file for appending

        if (SAMFile.is_open()) {
           SAMFile << "Connect Service Registry "<< std::endl;
           SAMFile.close();
       }*/
    }

    else if(strcmp(msg->getName(), "connectService") == 0)
    {
        EV << "MecAppBase::handleMessage- " << msg->getName() << endl;
        if(!serviceAddress.isUnspecified() && serviceSocket_->getState() != inet::TcpSocket::CONNECTED)
        {
            connect(serviceSocket_, serviceAddress, servicePort);

            /*SAMFile.open("mecapp.txt", std::ios::app);  // Open file for appending

            if (SAMFile.is_open()) {
                SAMFile << "The app connected to the service with service port: "<< servicePort << std::endl;
                SAMFile.close();
            }*/
        }
        else
        {
            if(serviceAddress.isUnspecified())
                EV << "MECSafetyApp::handleSelfMessage - service IP address is  unspecified (maybe response from the service registry is arriving)" << endl;
            else if(serviceSocket_->getState() == inet::TcpSocket::CONNECTED)
                EV << "MECSafetyApp::handleSelfMessage - service socket is already connected" << endl;
            auto nack = inet::makeShared<DataPacket>();
            // the connectService message is scheduled after a start mec app from the UE app, so I can
            // response to her here
            nack->setType(START_NACK);
            nack->setChunkLength(inet::B(2));
            inet::Packet* packet = new inet::Packet("WarningAlertPacketInfo");
            packet->insertAtBack(nack);
            //ueSocket.sendTo(packet, ueAppAddress, ueAppPort);

//            throw cRuntimeError("service socket already connected, or service IP address is unspecified");
        }
    }

    delete msg;
}

void MECSafetyApp::handleProcessedMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
     {
         handleSelfMessage(msg);
         return;
     }
    if (!msg->isSelfMessage()) {
        if(ueSocket.belongsToSocket(msg))
       {
           handleUeMessage(msg);
           delete msg;
           return;
       }
    }
    MecAppBase::handleProcessedMessage(msg);
    /*else {
            throw cRuntimeError("MECPingPongApp::handleprocessedmessage - not recognized message name %s",msg->getName());
        }*/
}
