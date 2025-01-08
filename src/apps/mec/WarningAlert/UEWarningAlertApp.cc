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

#include "apps/mec/WarningAlert/UEWarningAlertApp.h"

#include "../DeviceApp/DeviceAppMessages/DeviceAppPacket_m.h"
#include "../DeviceApp/DeviceAppMessages/DeviceAppPacket_Types.h"

#include "apps/mec/WarningAlert/packets/WarningAlertPacket_m.h"
#include "apps/mec/WarningAlert/packets/WarningAlertPacket_Types.h"

#include "inet/common/TimeTag_m.h"
#include "inet/common/packet/chunk/BytesChunk.h"

#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/transportlayer/common/L4PortTag_m.h"

#include "inet/mobility/base/MovingMobilityBase.h"

#include <fstream>
#include <iomanip>
#include <string>
#include <map>
#include <random>
#include <iostream>
#include <sys/stat.h>
#include "common/binder/Binder.h"


using namespace inet;
using namespace std;

std::ofstream UEFile;

Define_Module(UEWarningAlertApp);

UEWarningAlertApp::UEWarningAlertApp(){
    selfStart_ = NULL;
    selfStop_ = NULL;
}

UEWarningAlertApp::~UEWarningAlertApp(){
    cancelAndDelete(selfStart_);
    cancelAndDelete(selfStop_);
    cancelAndDelete(selfMecAppStart_);

}

inet::Coord getCoord_(cModule* mod)
{
    inet::IMobility *mobility_ = check_and_cast<inet::IMobility *>(mod->getSubmodule("mobility"));
    return mobility_->getCurrentPosition();
}

void UEWarningAlertApp::initialize(int stage)
{
    EV << "UEWarningAlertApp::initialize - stage " << stage << endl;
    cSimpleModule::initialize(stage);
    // avoid multiple initializations
    if (stage!=inet::INITSTAGE_APPLICATION_LAYER)
        return;

    log = par("logger").boolValue();

    //retrieve parameters
    size_ = par("packetSize");
    period_ = par("period");
    localPort_ = par("localPort");
    deviceAppPort_ = par("deviceAppPort");
    sourceSimbolicAddress = (char*)getParentModule()->getFullName();
    deviceSimbolicAppAddress_ = (char*)par("deviceAppAddress").stringValue();
    deviceAppAddress_ = inet::L3AddressResolver().resolve(deviceSimbolicAppAddress_);

    //binding socket
    socket.setOutputGate(gate("socketOut"));
    socket.bind(localPort_);

    int tos = par("tos");
    if (tos != -1)
        socket.setTos(tos);

    //retrieving car cModule
    ue = this->getParentModule();

    //retrieving mobility module
    cModule *temp = getParentModule()->getSubmodule("mobility");
    if(temp != NULL){
        mobility = check_and_cast<inet::IMobility*>(temp);
        //L2S-ESME
        //inet::IMobility *mobility_ = check_and_cast<inet::IMobility *>(temp);
        //mobility_->getCurrentPosition();
       inet::Coord coord = getCoord_(ue);

        UEFile.open("Vehicle.txt", std::ios::app);  // Open file for appending

         if (UEFile.is_open()) {
             UEFile << "Coordinates: ["<< coord.x << ";" << coord.y << "]" << std::endl;
             UEFile.close();
         }
        //L2S-ESME
    }
    else {
        EV << "UEWarningAlertApp::initialize - \tWARNING: Mobility module NOT FOUND!" << endl;
        throw cRuntimeError("UEWarningAlertApp::initialize - \tWARNING: Mobility module NOT FOUND!");
    }

    mecAppName = par("mecAppName").stringValue();

    //initializing the auto-scheduling messages
    selfStart_ = new cMessage("selfStart");
    selfStop_ = new cMessage("selfStop");
    selfMecAppStart_ = new cMessage("selfMecAppStart");

    //starting UEWarningAlertApp
    simtime_t startTime = par("startTime");
    EV << "UEWarningAlertApp::initialize - starting sendStartMEWarningAlertApp() in " << startTime << " seconds " << endl;
    scheduleAt(simTime() + startTime, selfStart_);

    //testing
    EV << "UEWarningAlertApp::initialize - sourceAddress: " << sourceSimbolicAddress << " [" << inet::L3AddressResolver().resolve(sourceSimbolicAddress).str()  <<"]"<< endl;
    EV << "UEWarningAlertApp::initialize - destAddress: " << deviceSimbolicAppAddress_ << " [" << deviceAppAddress_.str()  <<"]"<< endl;
    EV << "UEWarningAlertApp::initialize - binding to port: local:" << localPort_ << " , dest:" << deviceAppPort_ << endl;

    alertMECDelay_ = registerSignal("alertMECDelay");
    alertRcvdMsg_ = registerSignal("alertRcvdMsg");
    initDelay_ = registerSignal("initDelay");
    stopDelay_ = registerSignal("stopDelay");

    nrReceived = 0;
    delaySum = 0;

    initnrReceived = 0;
    initdelaySum = 0;

    stopnrReceived = 0;
    stopdelaySum = 0;
    Rimo =0;
}

double exponentialGen(double lambda) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::exponential_distribution<> exponential(lambda);
    double generated;
    generated = exponential(gen);
    return generated;
}


void UEWarningAlertApp::handleMessage(cMessage *msg)
{
    EV << "UEWarningAlertApp::handleMessage" << endl;
    // Sender Side
    if (msg->isSelfMessage())
    {
        if(!strcmp(msg->getName(), "selfStart"))   sendStartMEWarningAlertApp();

        else if(!strcmp(msg->getName(), "selfStop"))    sendStopMEWarningAlertApp();

        else if(!strcmp(msg->getName(), "selfMecAppStart"))
        {
            double time_to_wait_d ;
            time_to_wait_d= exponentialGen(10);
            simtime_t time_to_wait {time_to_wait_d};

            sendMessageToMECApp();

            scheduleAt(simTime() + 5, selfMecAppStart_);
        }

        else    throw cRuntimeError("UEWarningAlertApp::handleMessage - \tWARNING: Unrecognized self message");
    }
    // Receiver Side
    else{
        inet::Packet* packet = check_and_cast<inet::Packet*>(msg);

        inet::L3Address ipAdd = packet->getTag<L3AddressInd>()->getSrcAddress();
        // int port = packet->getTag<L4PortInd>()->getSrcPort();

        /*
         * From Device app
         * device app usually runs in the UE (loopback), but it could also run in other places
         */
        if(ipAdd == deviceAppAddress_ || ipAdd == inet::L3Address("127.0.0.1")) // dev app
        {
            auto mePkt = packet->peekAtFront<DeviceAppPacket>();

            if (mePkt == 0)
                throw cRuntimeError("UEWarningAlertApp::handleMessage - \tFATAL! Error when casting to DeviceAppPacket");

            if( !strcmp(mePkt->getType(), ACK_START_MECAPP) )    handleAckStartMEWarningAlertApp(msg);

            else if(!strcmp(mePkt->getType(), ACK_STOP_MECAPP))  handleAckStopMEWarningAlertApp(msg);

            else
            {
                throw cRuntimeError("UEWarningAlertApp::handleMessage - \tFATAL! Error, DeviceAppPacket type %s not recognized", mePkt->getType());
            }
        }
        // From MEC application
        else
        {
            auto mePkt = packet->peekAtFront<WarningAppPacket>();
            if (mePkt == 0)
                throw cRuntimeError("UEWarningAlertApp::handleMessage - \tFATAL! Error when casting to WarningAppPacket");

            if(!strcmp(mePkt->getType(), WARNING_ALERT))      handleInfoMEWarningAlertApp(msg);
            else if(!strcmp(mePkt->getType(), START_NACK))
            {
                EV << "UEWarningAlertApp::handleMessage - MEC app did not started correctly, trying to start again" << endl;
            }
            else if(!strcmp(mePkt->getType(), START_ACK))
            {
                EV << "UEWarningAlertApp::handleMessage - MEC app started correctly" << endl;
                if(selfMecAppStart_->isScheduled())
                {
                    cancelEvent(selfMecAppStart_);
                }
            }
            else
            {
                throw cRuntimeError("UEWarningAlertApp::handleMessage - \tFATAL! Error, WarningAppPacket type %s not recognized", mePkt->getType());
            }
        }
        delete msg;
    }
}

void UEWarningAlertApp::finish()
{

}
/*
 * -----------------------------------------------Sender Side------------------------------------------
 */
void UEWarningAlertApp::sendStartMEWarningAlertApp()
{
    inet::Packet* packet = new inet::Packet("WarningAlertPacketStart");
    auto start = inet::makeShared<DeviceAppStartPacket>();

    //instantiation requirements and info
    start->setType(START_MECAPP);
    start->setMecAppName(mecAppName.c_str());
    start->setPayloadTimestamp(simTime());
    Rimo = simTime();
    //start->setMecAppProvider("lte.apps.mec.warningAlert_rest.MEWarningAlertApp_rest_External");

    start->setChunkLength(inet::B(2+mecAppName.size()+1));
    start->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());

    packet->insertAtBack(start);

    socket.sendTo(packet, deviceAppAddress_, deviceAppPort_);

    if(log)
    {
        ofstream myfile;
        myfile.open ("example.txt", ios::app);
        if(myfile.is_open())
        {
            myfile <<"["<< NOW << "] UEWarningAlertApp - UE sent start message to the Device App \n";
            myfile.close();

        }
    }

    //rescheduling
    scheduleAt(simTime() + period_, selfStart_);
}
void UEWarningAlertApp::sendStopMEWarningAlertApp()
{
    EV << "UEWarningAlertApp::sendStopMEWarningAlertApp - Sending " << STOP_MEAPP <<" type WarningAlertPacket\n";

    inet::Packet* packet = new inet::Packet("DeviceAppStopPacket");
    auto stop = inet::makeShared<DeviceAppStopPacket>();

    //termination requirements and info
    stop->setType(STOP_MECAPP);

    stop->setChunkLength(inet::B(size_));
    stop->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());

    packet->insertAtBack(stop);
    socket.sendTo(packet, deviceAppAddress_, deviceAppPort_);

    if(log)
    {
        ofstream myfile;
        myfile.open ("example.txt", ios::app);
        if(myfile.is_open())
        {
            myfile <<"["<< NOW << "] UEWarningAlertApp - UE sent stop message to the Device App \n";
            myfile.close();
        }
    }

    //rescheduling
    if(selfStop_->isScheduled())
        cancelEvent(selfStop_);
    scheduleAt(simTime() + period_, selfStop_);

    UEFile.open("output.txt", std::ios::app);  // Open file for appending

    if (UEFile.is_open()) {
         UEFile << "The vehicle send stop MEC App to the device App" << std::endl;
         UEFile.close();
    }
}

/*
 * ---------------------------------------------Receiver Side------------------------------------------
 */
void UEWarningAlertApp::handleAckStartMEWarningAlertApp(cMessage* msg)
{
    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    auto pkt = packet->peekAtFront<DeviceAppStartAckPacket>();

    auto ackt = inet::makeShared<DeviceAppStartPacket>();


    if(pkt->getResult() == true)
    {
        UEFile.open("output.txt", std::ios::app);  // Open file for appending

        if (UEFile.is_open()) {
             UEFile << "The vehicle receives ACK at: "<< simTime() << std::endl;
             UEFile.close();
        }
        mecAppAddress_ = L3AddressResolver().resolve(pkt->getIpAddress());
        mecAppPort_ = pkt->getPort();
        EV << "UEWarningAlertApp::handleAckStartMEWarningAlertApp - Received " << pkt->getType() << " type WarningAlertPacket. mecApp isntance is at: "<< mecAppAddress_<< ":" << mecAppPort_ << endl;
        cancelEvent(selfStart_);
        //scheduling sendStopMEWarningAlertApp()
        if(!selfStop_->isScheduled()){
            simtime_t  stopTime = par("stopTime");
            scheduleAt(simTime() + stopTime, selfStop_);
            EV << "UEWarningAlertApp::handleAckStartMEWarningAlertApp - Starting sendStopMEWarningAlertApp() in " << stopTime << " seconds " << endl;
            UEFile.open("output.txt", std::ios::app);  // Open file for appending

            if (UEFile.is_open()) {
                 UEFile << "The vehicle start send stop MEC app" << std::endl;
                 UEFile.close();
            }
        }
        // emit statistics
         simtime_t initdelay = simTime() - 2;
         emit(initDelay_, initdelay);
         // emit(alertRcvdMsg_, (long)1);
         initnrReceived++;
         initdelaySum+=initdelay;

         EV <<"] initDelay[" << initdelay << "]" << endl;

         sendMessageToMECApp();
         scheduleAt(simTime() + period_, selfMecAppStart_);
    }

    else
    {
        EV << "UEWarningAlertApp::handleAckStartMEWarningAlertApp - MEC application cannot be instantiated! Reason: " << pkt->getReason() << endl;
    }

    //sendMessageToMECApp();
    //scheduleAt(simTime() + period_, selfMecAppStart_);

}



void UEWarningAlertApp::sendMessageToMECApp(){

    // send star monitoring message to the MEC application

    inet::Packet* pkt = new inet::Packet("WarningAlertPacketStart");
    auto alert = inet::makeShared<WarningStartPacket>();
    alert->setType(START_WARNING);
    alert->setCenterPositionX(par("positionX").doubleValue());
    alert->setCenterPositionY(par("positionY").doubleValue());
    alert->setRadius(par("radius").doubleValue());


    int payload;
    payload = exponentialGen(1.0 / 40000);
    payload = (payload>65527)? 65527:payload;
    payload = (payload<1)? 1:payload;
    EV<<"RRRRRRRRRRRRRRRRRRRRRRRRRRR"<<payload<<endl;

    alert->setChunkLength(inet::B(payload));
    // I add this line to read the start time
    alert->setPayloadTimestamp(simTime());
    alert->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
    pkt->insertAtBack(alert);

    if(log)
    {
        ofstream myfile;
        myfile.open ("example.txt", ios::app);
        if(myfile.is_open())
        {
            myfile <<"["<< NOW << "] UEWarningAlertApp - UE sent start subscription message to the MEC application \n";
            myfile.close();
        }
    }

    socket.sendTo(pkt, mecAppAddress_ , mecAppPort_);

    EV << "UEWarningAlertApp::sendMessageToMECApp() - start Message sent to the MEC app" << endl;
    UEFile.open("output.txt", std::ios::app);  // Open file for appending

     if (UEFile.is_open()) {
         UEFile << "UEWarningAlertApp::sendMessageToMECApp() - start Message sent to the MEC app: The address: "<< mecAppAddress_ << "and the port is: "<< mecAppPort_ << std::endl;
         UEFile << "UEWarningAlertApp::sendMessageToMECApp() at: "<< simTime()  << std::endl;

         UEFile.close();
     }
}

void UEWarningAlertApp::handleInfoMEWarningAlertApp(cMessage* msg)
{
    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    auto pkt = packet->peekAtFront<WarningAlertPacket>();

    EV << "UEWarningAlertApp::handleInfoMEWarningrAlertApp - Received " << pkt->getType() << " type WarningAlertPacket"<< endl;

    //updating runtime color of the car icon background
    if(pkt->getDanger())
    {
        // emit statistics
        simtime_t delay = simTime() - pkt->getPayloadTimestamp();
        emit(alertMECDelay_, delay);
        emit(alertRcvdMsg_, (long)1);
        nrReceived++;
        delaySum+=delay;

        EV <<"] alertMECDelay[" << delay << "]" << endl;

        EV <<"RIM:" << pkt->getPayloadTimestamp() << endl;
        EV <<" Hela:" << simTime() << endl;

        if(log)
        {
            ofstream myfile;
            myfile.open ("example.txt", ios::app);
            if(myfile.is_open())
            {
                myfile <<"["<< NOW << "] UEWarningAlertApp - UE received danger alert TRUE from MEC application \n";
                myfile.close();
            }
        }

        EV << "UEWarningAlertApp::handleInfoMEWarningrAlertApp - Warning Alert Detected: DANGER!" << endl;
        ue->getDisplayString().setTagArg("i",1, "red");
    }
    else{
        if(log)
        {
            ofstream myfile;
            myfile.open ("example.txt", ios::app);
            if(myfile.is_open())
            {
                myfile <<"["<< NOW << "] UEWarningAlertApp - UE received danger alert FALSE from MEC application \n";
                myfile.close();
            }
        }

        EV << "UEWarningAlertApp::handleInfoMEWarningrAlertApp - Warning Alert Detected: NO DANGER!" << endl;
        ue->getDisplayString().setTagArg("i",1, "green");
        // emit statistics
        simtime_t stopdelay = simTime() - pkt->getTimestamp();
        emit(stopDelay_, stopdelay);
        // emit(alertRcvdMsg_, (long)1);
        initnrReceived++;
        stopdelaySum+=stopdelay;

        EV <<"] stopDelay[" << stopdelay << "]" << endl;
    }
}
void UEWarningAlertApp::handleAckStopMEWarningAlertApp(cMessage* msg)
{

    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    auto pkt = packet->peekAtFront<DeviceAppStopAckPacket>();

    EV << "UEWarningAlertApp::handleAckStopMEWarningAlertApp - Received " << pkt->getType() << " type WarningAlertPacket with result: "<< pkt->getResult() << endl;
    if(pkt->getResult() == false)
        EV << "Reason: "<< pkt->getReason() << endl;
    //updating runtime color of the car icon background
    ue->getDisplayString().setTagArg("i",1, "white");

    cancelEvent(selfStop_);
}
