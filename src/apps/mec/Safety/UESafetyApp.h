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

#ifndef __UEWARNINGALERTAPP_H_
#define __UEWARNINGALERTAPP_H_

#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"

#include "common/binder/Binder.h"

//inet mobility
#include "inet/common/geometry/common/Coord.h"
#include "inet/common/geometry/common/EulerAngles.h"
#include "inet/mobility/contract/IMobility.h"

//WarningAlertPacket
#include "nodes/mec/MECPlatform/MEAppPacket_Types.h"
#include "apps/mec/Safety/packets/SafetyPacket_m.h"
#include "apps/mec/Safety/packets/SafetyPacket_Types.h"



//#include "spdlog/spdlog.h"  // logging library
//#include "spdlog/sinks/basic_file_sink.h"
#include <ctime>
//#include <fmt/format.h>



using namespace omnetpp;

/**
 * This is a UE app that asks to a Device App to instantiate the MECWarningAlertApp.
 * After a successful response, it asks to the MEC app to be notified when the car
 * enters a circle zone described by x,y center position and the radius. When a danger
 * event arrives, the car colors becomes red.
 *
 * The event behavior flow of the app is:
 * 1) send create MEC app to the Device App
 * 2.1) ACK --> send coordinates to MEC app
 * 2.2) NACK --> do nothing
 * 3) wait for danger events
 * 4) send delete MEC app to the Device App
 */

class UESafetyApp: public cSimpleModule
{


    static omnetpp::simsignal_t failed_to_start_mecApp_Signal;
    std::string log_level = "low";

    //communication to device app and mec app
    inet::UdpSocket socket;
    int nbRcvMsg;
    int size_;
    simtime_t period_;
    int localPort_;
    int deviceAppPort_;
    inet::L3Address deviceAppAddress_;


    //std::string App_name_;
    std::string UE_name_;
        //static  unsigned int iDframe_ ;



    double payloadSize_;
    double interReqTime_;
    //int test;


    char* sourceSimbolicAddress;            //Ue[x]
    char* deviceSimbolicAppAddress_;              //meHost.virtualisationInfrastructure

    // MEC application endPoint (returned by the device app)
    inet::L3Address mecAppAddress_;
    int mecAppPort_;

    std::string mecAppName;

    int nb_fails = 0;
//    std::string log_identifier;
    //double maxCpuSpeed_;


    //Defined to calculate the delay
    omnetpp::simsignal_t DLPacketDelay_;
    omnetpp::simsignal_t packetRcvdMsg_;
    omnetpp::simsignal_t initDelay_;
    omnetpp::simsignal_t stopDelay_;

    omnetpp::simtime_t delaySum;
    omnetpp::simtime_t initdelaySum;
    omnetpp::simtime_t stopdelaySum;

    omnetpp::simtime_t Rimo;

    long nrReceived;
    long initnrReceived;
    long stopnrReceived;


    double DataRate_;
    // mobility informations
    cModule* ue;
    inet::IMobility *mobility;
    inet::Coord position;

    //scheduling
    cMessage *selfStart_;
    cMessage *selfStop_;

    cMessage *selfMecAppStart_;
    cMessage *selfSendData_;

    // uses to write in a log a file
    bool log;
    bool mecApp_on;

   // int numCars;

   // std::shared_ptr<spdlog::logger> myLogger;
    //std::shared_ptr<spdlog::logger> Fails_Logger;



    public:
        ~UESafetyApp();
        UESafetyApp();

    protected:

        std::string csv_filename_fails;
        std::string csv_filename_total;
        std::string csv_filename_rcv_total;
        std::string csv_filename_send_total;



        virtual int numInitStages() const { return inet::NUM_INIT_STAGES; }
        void initialize(int stage);
        virtual void handleMessage(cMessage *msg);
        virtual void finish();

        void sendStartMEWarningAlertApp();
        //void sendMessageToMECApp();
        void sendStopMEWarningAlertApp();

        void sendDataPacket(simtime_t t);

        //L2S-ESME
        void sendCircleInformations();
        void handleNotificationMessage(cMessage* msg);
        //L2S-ESME

        void handleAckStartMEWarningAlertApp(cMessage* msg);
        void handleInfoMEWarningAlertApp(cMessage* msg);
        void handleAckStopMEWarningAlertApp(cMessage* msg);
};

#endif
