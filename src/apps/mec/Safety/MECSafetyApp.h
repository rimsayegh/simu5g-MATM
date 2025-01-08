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

#ifndef __MECWARNINGALERTAPP_H_
#define __MECWARNINGALERTAPP_H_

#include "omnetpp.h"

#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"

//MEWarningAlertPacket
//#include "nodes/mec/MECPlatform/MECAppPacket_Types.h"
#include "apps/mec/Safety/packets/SafetyPacket_Types.h"
#include "apps/mec/Safety/packets/SafetyPacket_m.h"

#include "nodes/mec/MECPlatform/ServiceRegistry/ServiceRegistry.h"

#include "apps/mec/MecApps/MecAppBase.h"


//#include "spdlog/spdlog.h"  // logging library
//#include "spdlog/sinks/basic_file_sink.h"
#include <ctime>
//#include <fmt/format.h>

#include  "apps/mec/MecApps/packets/ProcessingTimeMessage_m.h"

using namespace std;
using namespace omnetpp;

//
//  This is a simple MEC app that receives the coordinates and the radius from the UE app
//  and subscribes to the CircleNotificationSubscription of the Location Service. The latter
//  periodically checks if the UE is inside/outside the area and sends a notification to the
//  MEC App. It then notifies the UE.

//
//  The event behavior flow of the app is:
//  1) receive coordinates from the UE app
//  2) subscribe to the circleNotificationSubscription
//  3) receive the notification
//  4) send the alert event to the UE app
//  5) (optional) receive stop from the UE app
//
// TCP socket management is not fully controlled. It is assumed that connections works
// at the first time (The scenarios used to test the app are simple). If a deeper control
// is needed, feel free to improve it.

//

class MECSafetyApp : public MecAppBase
{

    //UDP socket to communicate with the UeApp
    inet::UdpSocket ueSocket;
    int localUePort;

    inet::L3Address ueAppAddress;
    int ueAppPort;

    inet::TcpSocket* serviceSocket_;
    inet::TcpSocket* mp1Socket_;

    HttpBaseMessage* mp1HttpMessage;
    HttpBaseMessage* serviceHttpMessage;

    int iDframe_;
    int size_;
    std::string subId;


    omnetpp::simsignal_t ULPacketDelay_;
    omnetpp::simsignal_t QueueingMECDelay_;
    omnetpp::simsignal_t processingMECDelay_;

    omnetpp::simsignal_t ULpacketRcvdMsg_;

    omnetpp::simtime_t ULdelaySum;
    omnetpp::simtime_t QdelaySum;
    omnetpp::simtime_t PdelaySum;


    long ULnrReceived;
    long QnrReceived;
    long PnrReceived;

    // circle danger zone
    //double Radius_;
    cOvalFigure * circle;
    double centerPositionX;
    double centerPositionY;
    double radius;


    static int counter;
    static omnetpp::simsignal_t mecapp_ul_delaySignal;
    static omnetpp::simsignal_t mecapp_pk_rcv_sizeSignal;

    int numCars;
    int row =0 ;
    double processingTime_;
    //double maxCpuSpeed_;
    //std::string App_name_;
    int nbSentMsg;
    cQueue pingPongpacketQueue_;
    cMessage* pingPongprocessMessage_;



    protected:
        virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
        virtual void initialize(int stage) override;
        virtual void finish() override;

        virtual void handleProcessedMessage(omnetpp::cMessage *msg) override;

        virtual void handleHttpMessage(int connId) override;
        virtual void handleServiceMessage(int connId) override;
        virtual void handleMp1Message(int connId) override;
        virtual void handleUeMessage(omnetpp::cMessage *msg) override;

        virtual void modifySubscription();
        virtual void sendSubscription();
        virtual void sendDeleteSubscription();

        virtual void handleSelfMessage(cMessage *msg) override;


        virtual void sendDataPacket(cMessage *msg);
        virtual double computationTime(int N);
        virtual double scheduleNextMsg(cMessage* msg) override;
        virtual void handleMessage(omnetpp::cMessage *msg) override;

        //void handleCriticalNotifications(); //Test
//        /* TCPSocket::CallbackInterface callback methods */
       virtual void established(int connId) override;

    public:
       MECSafetyApp();
       virtual ~MECSafetyApp();

};

#endif
