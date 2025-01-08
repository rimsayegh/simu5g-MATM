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

#include "apps/mec/WarningAlert/MECWarningAlertApp.h"

#include "apps/mec/DeviceApp/DeviceAppMessages/DeviceAppPacket_Types.h"
#include "apps/mec/WarningAlert/packets/WarningAlertPacket_Types.h"

#include "inet/common/TimeTag_m.h"
#include "inet/common/packet/Packet_m.h"

#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/transportlayer/common/L4PortTag_m.h"

#include "nodes/mec/utils/httpUtils/httpUtils.h"
#include "nodes/mec/utils/httpUtils/json.hpp"
#include "nodes/mec/MECPlatform/MECServices/packets/HttpResponseMessage/HttpResponseMessage.h"

#include <fstream>

Define_Module(MECWarningAlertApp);

std::ofstream mecFile;

using namespace inet;
using namespace omnetpp;

MECWarningAlertApp::MECWarningAlertApp(): MecAppBase()
{
    circle = nullptr; // circle danger zone

}
/*MECWarningAlertApp::~MECWarningAlertApp()
{
    if(circle != nullptr)
    {
        if(getSimulation()->getSystemModule()->getCanvas()->findFigure(circle) != -1)
            getSimulation()->getSystemModule()->getCanvas()->removeFigure(circle);
        delete circle;
    }

}*/
MECWarningAlertApp::~MECWarningAlertApp()
{
    // Remove and delete the circle figure if it exists
    if (circle != nullptr)
    {
        if (getSimulation()->getSystemModule()->getCanvas()->findFigure(circle) != -1)
            getSimulation()->getSystemModule()->getCanvas()->removeFigure(circle);
        delete circle;
    }

    // Ensure that any self-messages are canceled and deleted
   /* if (mp1Socket_ != nullptr) {
        mp1Socket_->close(); // Close the socket connection
        delete mp1Socket_;    // Delete the mp1Socket
        mp1Socket_ = nullptr; // Set to nullptr to avoid future use
    }

    if (serviceSocket_ != nullptr) {
        serviceSocket_->close(); // Close the socket connection
        delete serviceSocket_;    // Delete the serviceSocket
        serviceSocket_ = nullptr; // Set to nullptr to avoid future use
    }*/

    // Check if there are any pending messages in a queue or a scheduled self-message
    /*if (!processedHttpMsgQueue.isEmpty()) {
        while (!processedHttpMsgQueue.isEmpty()) {
            delete processedHttpMsgQueue.pop();  // Delete any pending messages in the queue
        }
    }

    // Ensure any remaining scheduled self-messages are canceled and deleted
    cancelAndDelete(processingTimeMessage);  // Ensure we cancel and delete any pending ProcessingTimeMessage
    cancelAndDelete(subscriptionTimer);      // Clean up other messages like timers
*/
    EV << "MECWarningAlertApp::~MECWarningAlertApp() - Cleanup complete." << endl;
}



void MECWarningAlertApp::initialize(int stage)
{
    MecAppBase::initialize(stage);

    // avoid multiple initializations
    if (stage!=inet::INITSTAGE_APPLICATION_LAYER)
        return;

    //retrieving parameters
    size_ = par("packetSize");

    // set Udp Socket
    ueSocket.setOutputGate(gate("socketOut"));

    localUePort = par("localUePort");
    ueSocket.bind(localUePort);


    startAppDelay_ = registerSignal("startAppDelay");

    startdelaySum = 0;

    startnrReceived = 0;

    //testing
    EV << "MECWarningAlertApp::initialize - Mec application "<< getClassName() << " with mecAppId["<< mecAppId << "] has started!" << endl;

    mp1Socket_ = addNewSocket();


    // connect with the service registry
    cMessage *msg = new cMessage("connectMp1");
    scheduleAt(simTime() + 0, msg);

}

//void MECWarningAlertApp::handleMessage(cMessage *msg)
//{
////        MecAppBase::handleMessage(msg);
//    if (!msg->isSelfMessage())
//    {
//        if(ueSocket.belongsToSocket(msg))
//        {
//            handleUeMessage(msg);
//            delete msg;
//            return;
//        }
//    }
//    MecAppBase::handleMessage(msg);
//
//}

void MECWarningAlertApp::finish(){
    MecAppBase::finish();
    EV << "MECWarningAlertApp::finish()" << endl;

    if(gate("socketOut")->isConnected()){

    }
}

void MECWarningAlertApp::handleUeMessage(omnetpp::cMessage *msg)
{
    // determine its source address/port
    auto pk = check_and_cast<Packet *>(msg);
    ueAppAddress = pk->getTag<L3AddressInd>()->getSrcAddress();
    ueAppPort = pk->getTag<L4PortInd>()->getSrcPort();

    auto mecPk = pk->peekAtFront<WarningAppPacket>();

    if(strcmp(mecPk->getType(), START_WARNING) == 0)
    {
        /*
         * Read center and radius from message
         */
        EV << "MECWarningAlertApp::handleUeMessage - WarningStartPacket arrived" << endl;
        auto warnPk = dynamicPtrCast<const WarningStartPacket>(mecPk);
        if(warnPk == nullptr)
            throw cRuntimeError("MECWarningAlertApp::handleUeMessage - WarningStartPacket is null");

        centerPositionX = warnPk->getCenterPositionX();
        centerPositionY = warnPk->getCenterPositionY();
        radius = warnPk->getRadius();

        // emit statistics
        simtime_t startdelay = simTime() - warnPk->getPayloadTimestamp();
        emit(startAppDelay_, startdelay);
        startnrReceived++;
        startdelaySum+=startdelay;

        EV <<"] startAppDelay[" << startdelay << "]" << endl;

        if(par("logger").boolValue())
        {
            ofstream myfile;
            myfile.open ("example.txt", ios::app);
            if(myfile.is_open())
            {
                myfile <<"["<< NOW << "] MEWarningAlertApp - Received message from UE, connecting to the Location Service\n";
                myfile.close();
            }
        }

        cModule* mec = this->getParentModule();
        mecFile.open("mecapp.txt", std::ios::app);  // Open file for appending

         if (mecFile.is_open()) {
             mecFile << "MEWarningAlertApp - Received message from UEs:"<< mecAppId << "and the mecHost is: " << mec << std::endl;
             mecFile << "MEWarningAlertApp - Received message from UEs at: "<< simTime() << std::endl;
             mecFile.close();
         }

        cMessage *m = new cMessage("connectService");
        scheduleAt(simTime()+0.005, m);
    }
    else if (strcmp(mecPk->getType(), STOP_WARNING) == 0)
    {
        sendDeleteSubscription();
        mecFile.open("mecapp.txt", std::ios::app);  // Open file for appending

        if (mecFile.is_open()) {
            mecFile << "MEWarningAlertApp - Send Delete Subscription "<< std::endl;
            mecFile.close();
        }

    }

    else
    {
        throw cRuntimeError("MECWarningAlertApp::handleUeMessage - packet not recognized");
        mecFile.open("mecapp.txt", std::ios::app);  // Open file for appending

         if (mecFile.is_open()) {
             mecFile << "MEWarningAlertApp - Received message from UEs: Error" << std::endl;
             mecFile.close();
         }
    }
}

void MECWarningAlertApp::modifySubscription()
{
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

void MECWarningAlertApp::sendSubscription()
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
                            "\"mecAppId\": \"" + std::to_string(mecAppId) + "\""     //L2S-ESME //Reading the "mecAppId"
                            "}"
                            "}\r\n";
    std::string uri = "/example/location/v2/subscriptions/area/circle";
    std::string host = serviceSocket_->getRemoteAddress().str()+":"+std::to_string(serviceSocket_->getRemotePort());

    //Test //L2S-ESME
     mecFile.open("mecapp.txt", std::ios::app);  // Open file for appending

     if (mecFile.is_open()) {
         mecFile << "Test:"<< mecAppId << std::endl;
         mecFile.close();
     }
     //Test
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
}

void MECWarningAlertApp::sendDeleteSubscription()
{
    std::string uri = "/example/location/v2/subscriptions/area/circle/" + subId;
    std::string host = serviceSocket_->getRemoteAddress().str()+":"+std::to_string(serviceSocket_->getRemotePort());
    Http::sendDeleteRequest(serviceSocket_, host.c_str(), uri.c_str());
}

void MECWarningAlertApp::established(int connId)
{
    mecFile.open("mecapp.txt", std::ios::app);  // Open file for appending

    if (mecFile.is_open()) {
        mecFile << " established "<< std::endl;
        mecFile.close();
    }

    if(connId == mp1Socket_->getSocketId())
    {
        EV << "MECWarningAlertApp::established - Mp1Socket"<< endl;
        // get endPoint of the required service
        const char *uri = "/example/mec_service_mgmt/v1/services?ser_name=LocationService";
        std::string host = mp1Socket_->getRemoteAddress().str()+":"+std::to_string(mp1Socket_->getRemotePort());

        Http::sendGetRequest(mp1Socket_, host.c_str(), uri);
        return;
    }
    else if (connId == serviceSocket_->getSocketId())
    {
        EV << "MECWarningAlertApp::established - serviceSocket"<< endl;
        // the connectService message is scheduled after a start mec app from the UE app, so I can
        // response to her here, once the socket is established
        auto ack = inet::makeShared<WarningAppPacket>();
        ack->setType(START_ACK);
        ack->setChunkLength(inet::B(2));
        inet::Packet* packet = new inet::Packet("WarningAlertPacketInfo");
        packet->insertAtBack(ack);
        ueSocket.sendTo(packet, ueAppAddress, ueAppPort);
        sendSubscription();

        mecFile.open("mecapp.txt", std::ios::app);  // Open file for appending

        if (mecFile.is_open()) {
            mecFile << " send Subscription"<< std::endl;
            mecFile.close();
        }
    }
    else
    {
        throw cRuntimeError("MecAppBase::socketEstablished - Socket %d not recognized", connId);
    }
}


void MECWarningAlertApp::handleHttpMessage(int connId)
{
    mecFile.open("mecapp.txt", std::ios::app);  // Open file for appending

    if (mecFile.is_open()) {
        mecFile << " In handleHttpMessage "<< std::endl;
        mecFile.close();
    }

    if (mp1Socket_ != nullptr && connId == mp1Socket_->getSocketId()) {
        handleMp1Message(connId);

        mecFile.open("mecapp.txt", std::ios::app);  // Open file for appending

        if (mecFile.is_open()) {
            mecFile << " Handle Mp1 Message "<< std::endl;
            mecFile.close();
        }
    }
    else
    {
        handleServiceMessage(connId);

        mecFile.open("mecapp.txt", std::ios::app);  // Open file for appending

        if (mecFile.is_open()) {
            mecFile << " Handle Service Message "<< std::endl;
            mecFile.close();
        }
    }
}

void MECWarningAlertApp::handleMp1Message(int connId)
{
    // for now I only have just one Service Registry
    HttpMessageStatus *msgStatus = (HttpMessageStatus*) mp1Socket_->getUserData();
    mp1HttpMessage = (HttpBaseMessage*) msgStatus->httpMessageQueue.front();
    EV << "MECWarningAlertApp::handleMp1Message - payload: " << mp1HttpMessage->getBody() << endl;

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

                    mecFile.open("mecapp.txt", std::ios::app);  // Open file for appending

                    if (mecFile.is_open()) {
                        mecFile << " Connect to Location Service "<< std::endl;
                        mecFile.close();
                    }
                }
            }
            else
            {
                EV << "MECWarningAlertApp::handleMp1Message - LocationService not found"<< endl;
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

void MECWarningAlertApp::handleServiceMessage(int connId)
{
    HttpMessageStatus *msgStatus = (HttpMessageStatus*) serviceSocket_->getUserData();
    serviceHttpMessage = (HttpBaseMessage*) msgStatus->httpMessageQueue.front();

    mecFile.open("mecapp.txt", std::ios::app);  // Open file for appending

    if (mecFile.is_open()) {
        mecFile << "serviceHttpMessage type: " << serviceHttpMessage->getType() << std::endl;
        mecFile.close();
    }

    if(serviceHttpMessage->getType() == REQUEST)
    {
        mecFile.open("mecapp.txt", std::ios::app);  // Open file for appending

        if (mecFile.is_open()) {
            mecFile << "serviceHttpMessage name: " << serviceHttpMessage->getName() << std::endl;
            mecFile.close();
        }

        Http::send204Response(serviceSocket_); // send back 204 no content
        nlohmann::json jsonBody;
        EV << "MECWarningAlertApp::handleTcpMsg - REQUEST " << serviceHttpMessage->getBody()<< endl;
        try
        {
           jsonBody = nlohmann::json::parse(serviceHttpMessage->getBody()); // get the JSON structure
        }
        catch(nlohmann::detail::parse_error e)
        {
           std::cout  <<  e.what() << std::endl;
           // body is not correctly formatted in JSON, manage it
           return;
        }

        if(jsonBody.contains("subscriptionNotification"))
        {
            if(jsonBody["subscriptionNotification"].contains("enteringLeavingCriteria"))
            {
                nlohmann::json criteria = jsonBody["subscriptionNotification"]["enteringLeavingCriteria"] ;
                auto alert = inet::makeShared<WarningAlertPacket>();
                alert->setType(WARNING_ALERT);

                if(criteria == "Entering")
                {
                    EV << "MECWarningAlertApp::handleTcpMsg - Ue is Entered in the danger zone "<< endl;
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

                    mecFile.open("mecapp.txt", std::ios::app);  // Open file for appending

                    if (mecFile.is_open()) {
                        mecFile << " modify Subscription "<< std::endl;
                        mecFile.close();
                    }

                }
                else if (criteria == "Leaving")
                {
                    EV << "MECWarningAlertApp::handleTcpMsg - Ue left from the danger zone "<< endl;
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
                }

                alert->setPositionX(jsonBody["subscriptionNotification"]["terminalLocationList"]["currentLocation"]["x"]);
                alert->setPositionY(jsonBody["subscriptionNotification"]["terminalLocationList"]["currentLocation"]["y"]);
                alert->setChunkLength(inet::B(size_));
                alert->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());

                inet::Packet* packet = new inet::Packet("WarningAlertPacketInfo");
                packet->insertAtBack(alert);
                ueSocket.sendTo(packet, ueAppAddress, ueAppPort);

            }
        }
    }
    else if(serviceHttpMessage->getType() == RESPONSE)
    {
        mecFile.open("mecapp.txt", std::ios::app);  // Open file for appending

        if (mecFile.is_open()) {
            mecFile << "serviceHttpMessage name: " << serviceHttpMessage->getName() << std::endl;
            mecFile.close();
        }

        HttpResponseMessage *rspMsg = dynamic_cast<HttpResponseMessage*>(serviceHttpMessage);

        if(rspMsg->getCode() == 204) // in response to a DELETE
        {
            EV << "MECWarningAlertApp::handleTcpMsg - response 204, removing circle" << rspMsg->getBody()<< endl;
             serviceSocket_->close();
             getSimulation()->getSystemModule()->getCanvas()->removeFigure(circle);

        }
        else if(rspMsg->getCode() == 201) // in response to a POST
        {
            nlohmann::json jsonBody;
            EV << "MECWarningAlertApp::handleTcpMsg - response 201 " << rspMsg->getBody()<< endl;
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

        }
    }

}

void MECWarningAlertApp::handleSelfMessage(cMessage *msg)
{
    if(strcmp(msg->getName(), "connectMp1") == 0)
    {
        EV << "MecAppBase::handleMessage- " << msg->getName() << endl;
        connect(mp1Socket_, mp1Address, mp1Port);
    }

    else if(strcmp(msg->getName(), "connectService") == 0)
    {
        EV << "MecAppBase::handleMessage- " << msg->getName() << endl;
        if(!serviceAddress.isUnspecified() && serviceSocket_->getState() != inet::TcpSocket::CONNECTED)
        {
            connect(serviceSocket_, serviceAddress, servicePort);

            mecFile.open("mecapp.txt", std::ios::app);  // Open file for appending

            if (mecFile.is_open()) {
                mecFile << " In handleSelfMessage "<< std::endl;
                mecFile.close();
            }
        }
        else
        {
            if(serviceAddress.isUnspecified())
                EV << "MECWarningAlertApp::handleSelfMessage - service IP address is  unspecified (maybe response from the service registry is arriving)" << endl;
            else if(serviceSocket_->getState() == inet::TcpSocket::CONNECTED)
                EV << "MECWarningAlertApp::handleSelfMessage - service socket is already connected" << endl;
            auto nack = inet::makeShared<WarningAppPacket>();
            // the connectService message is scheduled after a start mec app from the UE app, so I can
            // response to her here
            nack->setType(START_NACK);
            nack->setChunkLength(inet::B(2));
            inet::Packet* packet = new inet::Packet("WarningAlertPacketInfo");
            packet->insertAtBack(nack);
            ueSocket.sendTo(packet, ueAppAddress, ueAppPort);

//            throw cRuntimeError("service socket already connected, or service IP address is unspecified");
        }
    }

    delete msg;
}

void MECWarningAlertApp::handleProcessedMessage(cMessage *msg)
{
    if (!msg->isSelfMessage()) {
        if(ueSocket.belongsToSocket(msg))
       {
           handleUeMessage(msg);
           delete msg;
           return;
       }
    }
    MecAppBase::handleProcessedMessage(msg);
}

