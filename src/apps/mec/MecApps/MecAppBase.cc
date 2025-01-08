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

#include "apps/mec/MecApps/MecAppBase.h"
#include "nodes/mec/utils/httpUtils/httpUtils.h"
#include "nodes/mec/utils/MecCommon.h"
#include  "apps/mec/MecApps/packets/ProcessingTimeMessage_m.h"

#include "common/utils/utils.h"

#include "inet/common/ProtocolTag_m.h"
#include "inet/common/ProtocolGroup.h"
#include "inet/common/Protocol.h"

#include <fstream>
#include <iomanip>
#include <string>
#include <map>
#include <random>
#include <iostream>
#include <sys/stat.h>
#include "common/binder/Binder.h"


using namespace omnetpp;
using namespace inet;
using namespace std;

MecAppBase::MecAppBase()
{
    mecHost = nullptr;
    mecPlatform = nullptr;;
    serviceRegistry = nullptr;
    sendTimer = nullptr;
    vim = nullptr;
    processMessage_ = nullptr;
    currentProcessedMsg_ = nullptr;
}

MecAppBase::~MecAppBase()
{
    std::cout << "MecAppBase::~MecAppBase()" << std::endl;
    if(sendTimer != nullptr)
    {
        if(sendTimer->isSelfMessage())
            cancelAndDelete(sendTimer);
        else
            delete sendTimer;
    }

    // delete all the messages
    for(auto &sock : sockets_.getMap())
    {

        TcpSocket* tcpSock = (TcpSocket*)sock.second;
       // removeSocket(tcpSock);
    }
    // it calls delete, too
    sockets_.deleteSockets();

    cancelAndDelete(processMessage_);
}

void MecAppBase::initialize(int stage)
{

    if(stage != inet::INITSTAGE_APPLICATION_LAYER)
        return;

    const char *mp1Ip = par("mp1Address");
    mp1Address = L3AddressResolver().resolve(mp1Ip);
    mp1Port = par("mp1Port");

//    TcpSocket* serviceSocket = new TcpSocket();
//    serviceSocket->setOutputGate(gate("socketOut"));
//    serviceSocket->setCallback(this);
//    HttpMessageStatus* msg = new HttpMessageStatus;
//    msg = nullptr;
//    serviceSocket->setUserData(msg);
//
//    sockets_["serviceSocket"] = serviceSocket;
//
//    TcpSocket* mp1Socket = new TcpSocket();
//    mp1Socket->setOutputGate(gate("socketOut"));
//    mp1Socket->setCallback(this);
//    HttpMessageStatus* msg1 = new HttpMessageStatus;
//    msg1 = nullptr;
//    mp1Socket->setUserData(msg1);
//
//    sockets_["mp1Socket"] = mp1Socket;

    mecAppId = par("mecAppId"); // FIXME mecAppId is the deviceAppId (it does not change anything, though)
    mecAppIndex_ = par("mecAppIndex");
    requiredRam = par("requiredRam").doubleValue();
    requiredDisk = par("requiredDisk").doubleValue();
    requiredCpu = par("requiredCpu").doubleValue();

    vim = check_and_cast<VirtualisationInfrastructureManager*>(getParentModule()->getSubmodule("vim"));
    mecHost = getParentModule();
    mecPlatform = mecHost->getSubmodule("mecPlatform");

    serviceRegistry = check_and_cast<ServiceRegistry *>(mecPlatform->getSubmodule("serviceRegistry"));

    processMessage_ = new cMessage("processedMessage");



   //Added by ESME-L2S
    processingDelay_ = registerSignal("processingDelay");

    processingDelaySum = 0;

    pnrReceived = 1;

    temp=0;
}

void MecAppBase::connect(inet::TcpSocket* socket, const inet::L3Address& address, const int port)
{
    // we need a new connId if this is not the first connection
    //socket->renewSocket();

    int timeToLive = par("timeToLive");
    if (timeToLive != -1)
        socket->setTimeToLive(timeToLive);

    int dscp = par("dscp");
    if (dscp != -1)
        socket->setDscp(dscp);

    int tos = par("tos");
    if (tos != -1)
        socket->setTos(tos);

    if (address.isUnspecified()) {
        EV_ERROR << "Connecting to " << address << " port=" << port << ": cannot resolve destination address\n";
    }
    else {
        EV_INFO << "Connecting to " << address << " port=" << port << endl;

        socket->connect(address, port);
    }
}

void MecAppBase::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())

    {
        EV <<" HelloS" << endl;
        if(strcmp(msg->getName(), "processedMessage") == 0)
        {
            EV <<" Hello11" << endl;
            handleProcessedMessage((cMessage*)packetQueue_.pop());
            //delete packetQueue_.pop();
            if(!packetQueue_.isEmpty())
            {
                double processingTime = scheduleNextMsg((cMessage *)packetQueue_.front());
                EV <<"MecAppBase::scheduleNextMsg() - next msg is processed in " << processingTime << "s" << endl;
                scheduleAt(simTime() + processingTime, processMessage_);
                temp++;

//                //Added by ESME-L2S
                //emit(processingDelay_, processingTime);
                processingDelaySum+=processingTime;
                EV <<" Hello111" << processingDelaySum << endl;
//                //
            }
            else
            {
                EV << "MecAppBase::handleMessage - no more messages are present in the queue" << endl;
                EV <<" Hello1111111" << endl;
            }
        }
        else if(strcmp(msg->getName(), "processedHttpMsg") == 0)
        {
            EV <<" Hello22" << endl;
            EV << "MecAppBase::handleMessage(): processedHttpMsg " << endl;
            ProcessingTimeMessage * procMsg = check_and_cast<ProcessingTimeMessage*>(msg);
            int connId = procMsg->getSocketId();
            TcpSocket* sock = (TcpSocket*)sockets_.getSocketById(connId);
            if(sock != nullptr)
            {
                EV <<" Hello222" << endl;
                HttpMessageStatus* msgStatus = (HttpMessageStatus *)sock->getUserData();
                handleHttpMessage(connId);
                delete msgStatus->httpMessageQueue.pop();
                if(!msgStatus->httpMessageQueue.isEmpty())
                {
                    EV << "MecAppBase::handleMessage(): processedHttpMsg - the httpMessageQueue is not empty, schedule next HTTP message" << endl;
                    double time = vim->calculateProcessingTime(mecAppId, 150);
                    scheduleAt(simTime()+time, msg);
                    temp++;
                    EV <<" Hello2222"<<mecAppId << endl;
//                    //Added by ESME-L2S
                    //emit(processingDelay_, time);
                    processingDelaySum+=time;
                    EV <<" Hello22221"<<processingDelaySum << endl;
//                    //
                }
            }
        }
        else
        {
            EV <<" HelloNO2" << endl;
            handleSelfMessage(msg);
        }
    }
    else

    {
        EV <<" HelloNOS" << endl;
        if(!processMessage_->isScheduled() && packetQueue_.isEmpty())
        {
            EV <<" Hello3" << endl;
            packetQueue_.insert(msg);
            double processingTime;
            if(strcmp(msg->getFullName(), "data") == 0)
            {
                EV <<" Hello33" << endl;
                processingTime = MecAppBase::scheduleNextMsg(msg);
                temp++;
            }
            else
            {
               EV <<" Hello333" << endl;
               processingTime = scheduleNextMsg(msg);
               temp++;
            }
            EV <<"MecAppBase::scheduleNextMsg() - next msg is processed in " << processingTime << "s" << endl;
            scheduleAt(simTime() + processingTime, processMessage_);
            EV <<" Hello4" <<simTime() + processingTime<<"s"<< endl;
//            //Added by ESME-L2S
            //emit(processingDelay_, processingTime);
            processingDelaySum+=processingTime;
            EV <<" Hello4" <<processingDelaySum<<"s"<< endl;
//            //

        }
        else if (processMessage_->isScheduled() && !packetQueue_.isEmpty())
        {
            EV <<" Hello42" << endl;
            packetQueue_.insert(msg);
        }
        else
        {
            EV <<" Hello43" << endl;
            throw cRuntimeError("MecAppBase::handleMessage - This situation is not possible");
        }
    }
    EV<<"temp: "<< temp<<endl;
}

//double exponentialGenerator(double lambda) {
//    std::random_device rd;
//    std::mt19937 gen(rd());
//    std::exponential_distribution<> exponential(lambda);
//    double generated;
//    generated = exponential(gen);
//    return generated;
//}

double MecAppBase::scheduleNextMsg(cMessage* msg)
{
//    // determine its source address/port
//
//    if (!strcmp(msg->getName(), "WarningAlertPacketStart"))
//            {
//        try {
//
//        double ins;
//        ins = exponentialGenerator(1.0 / (200*pow(10, 6)));
//        ins = (ins<0)? 0:ins;
//        double processingTime = vim->calculateProcessingTime(mecAppId, ins);
//        EV<<"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"<<processingTime<<endl;
//
//        return processingTime;
//
//            }
//
//        catch (const cRuntimeError& error){
//                //myLogger->info(fmt::format("ERROR {}///{}///{}///{}///{}///{}///{}///",error.what(),error.what(),error.what(),error.what(),error.what(),error.what(),error.what()));
//                delete msg;
//            }
//
//            }
//    else if (!strcmp(msg->getName(), "RealTimeVideoStreamingAppSegment")) {
//        double INS;
//        int frameSize;
//        frameSize = 5000;
//        INS = frameSize * 2;
//        double processingTime = vim->calculateProcessingTime(mecAppId, INS);
//        EV<<"VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV"<<processingTime<<endl;
//        //processingVideoDelaySum+=processingTime;
//
//
//        return processingTime;
//    }
//    else {
//        double processingTime = vim->calculateProcessingTime(mecAppId, 20);// int(uniform(200, 400))); this cause machines to invert orders!
//        EV<<"mecAppId "<<mecAppId<<endl;
//        return processingTime;
//    }
//

    double processingTime = vim->calculateProcessingTime(mecAppId, 30);// int(uniform(200, 400))); this cause machines to invert orders!
    EV<<"mecAppId "<<mecAppId<<endl;
    return processingTime;
}

void MecAppBase::handleProcessedMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
    {
        handleSelfMessage(msg);
    }
    else
    {
        TcpSocket* sock = (TcpSocket*)sockets_.findSocketFor(msg);
        if(sock != nullptr)
        {
            EV << "MecAppBase::handleProcessedMessage(): message for socket with ID: " << sock->getSocketId() << endl;
            sock->processMessage(msg);
        }
        else
        {
            delete msg;
        }
    }
}

void MecAppBase::socketEstablished(TcpSocket * socket)
{
    established(socket->getSocketId());
}

void MecAppBase::socketDataArrived(inet::TcpSocket *socket, inet::Packet *msg, bool)
{
    EV << "MecAppBase::socketDataArrived" << endl;

//  In progress. Trying to handle an app message coming from different tcp segments in a better way
//    bool res = msg->hasAtFront<HttpResponseMessage>();
//    auto pkc = msg->peekAtFront<HttpResponseMessage>(b(-1), Chunk::PF_ALLOW_EMPTY | Chunk::PF_ALLOW_SERIALIZATION | Chunk::PF_ALLOW_INCOMPLETE);
//    if(pkc!=nullptr && pkc->isComplete() &&  !pkc->isEmpty()){
//
//    }
//    else
//    {
//        EV << "Null"<< endl;
//    }


//
//    EV << "MecAppBase::socketDataArrived - payload: " << endl;
//

    std::vector<uint8_t> bytes =  msg->peekDataAsBytes()->getBytes();
    std::string packet(bytes.begin(), bytes.end());
//    EV << packet << endl;
    HttpMessageStatus* msgStatus = (HttpMessageStatus*) socket->getUserData();

    bool res =  Http::parseReceivedMsg(socket->getSocketId(),  packet, msgStatus->httpMessageQueue , &(msgStatus->bufferedData),  &(msgStatus->currentMessage));
    if(res)
    {
        //msgStatus->currentMessage->setSockId(socket->getSocketId());
        if(vim == nullptr)
            throw cRuntimeError("MecAppBase::socketDataArrived - vim is null!");
        double time = vim->calculateProcessingTime(mecAppId, 150);
        temp++;
        EV <<" Test 4" << endl;
//        //Added by ESME-L2S
        //emit(processingDelay_, time);
        processingDelaySum+=time;
        EV <<" Test 4" << processingDelaySum << endl;
//               //
        if(!msgStatus->processMsgTimer->isScheduled())
            scheduleAt(simTime()+time, msgStatus->processMsgTimer);

    }

    EV<<"temp: "<< temp<<endl;

    delete msg;
}

void MecAppBase::socketPeerClosed(TcpSocket *socket_)
{
    EV << "MecAppBase::socketPeerClosed" << endl;
    socket_->close();
}

void MecAppBase::socketClosed(TcpSocket *socket)
{
    EV_INFO << "MecAppBase::socketClosed" << endl;
}

void MecAppBase::socketFailure(TcpSocket *sock, int code)
{
    // subclasses may override this function, and add code try to reconnect after a delay.
}


TcpSocket* MecAppBase::addNewSocket()
{
    TcpSocket* newSocket = new TcpSocket();
    newSocket->setOutputGate(gate("socketOut"));
    newSocket->setCallback(this);
    HttpMessageStatus* msg = new HttpMessageStatus;
    msg->processMsgTimer = new ProcessingTimeMessage("processedHttpMsg");
    msg->processMsgTimer->setSocketId(newSocket->getSocketId());
    newSocket->setUserData(msg);

    sockets_.addSocket(newSocket);
    EV << "MecAppBase::addNewSocket(): added socket with ID: " << newSocket->getSocketId() << endl;
    return newSocket;
}


void MecAppBase::removeSocket(inet::TcpSocket* tcpSock)
{
    HttpMessageStatus* msgStatus = (HttpMessageStatus *) tcpSock->getUserData();
    std::cout << "Deleting httpMessages in socket with sockId " << tcpSock->getSocketId() << std::endl;
    while(!msgStatus->httpMessageQueue.isEmpty())
    {
        std::cout << "Deleting httpMessages message" << std::endl;
        delete msgStatus->httpMessageQueue.pop();
    }
    if(msgStatus->currentMessage != nullptr )
        delete msgStatus->currentMessage;
    if(msgStatus->processMsgTimer != nullptr)
    {
        cancelAndDelete(msgStatus->processMsgTimer);
    }
    delete sockets_.removeSocket(tcpSock);
}

void MecAppBase::finish()
{
    EV << "MecAppBase::finish()" << endl;
    emit(processingDelay_, processingDelaySum);
//    if(serviceSocket_.getState() == inet::TcpSocket::CONNECTED)
//        serviceSocket_.close();
//    if(mp1Socket_.getState() == inet::TcpSocket::CONNECTED)
//        mp1Socket_.close();

}





