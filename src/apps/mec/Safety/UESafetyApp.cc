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
#include "apps/mec/Safety/UESafetyApp.h"

#include "../DeviceApp/DeviceAppMessages/DeviceAppPacket_m.h"
#include "../DeviceApp/DeviceAppMessages/DeviceAppPacket_Types.h"

#include "apps/mec/Safety/packets/SafetyPacket_m.h"
#include "apps/mec/Safety/packets/SafetyPacket_Types.h"

#include "inet/common/TimeTag_m.h"
#include "inet/common/packet/chunk/BytesChunk.h"

#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/transportlayer/common/L4PortTag_m.h"


#include <fstream>

#include "UESafetyApp.h"

//#include "spdlog/spdlog.h"  // logging library
//#include "spdlog/sinks/basic_file_sink.h"
#include <ctime>
//#include <fmt/format.h>
//#include <boost/format.hpp>

#include <iomanip>
#include <string>
#include <map>
#include <random>
#include <iostream>
#include <vector>
#include <sys/stat.h>
#include "common/binder/Binder.h"


#include "CommonFunctions.h"

using namespace inet;
using namespace std;

std::ofstream SAFile;
std::ofstream dFile;
std::ofstream csvdata;

Define_Module(UESafetyApp);

simsignal_t UESafetyApp::failed_to_start_mecApp_Signal =registerSignal("failed_to_start_mecApp_Signal") ;


UESafetyApp::UESafetyApp(){
    selfStart_ = NULL;
    selfStop_ = NULL;
}

UESafetyApp::~UESafetyApp(){
    cancelAndDelete(selfStart_);
    cancelAndDelete(selfStop_);
    cancelAndDelete(selfMecAppStart_);

}

inet::Coord getcoord(cModule* mod)
{
    inet::IMobility *mobility_ = check_and_cast<inet::IMobility *>(mod->getSubmodule("mobility"));
    return mobility_->getCurrentPosition();
}

void writetoCSV(simtime_t time, double Vehicle_x, double Vehicle_y, double Speed, double DataRate) {
    csvdata.open("Info_log.csv", std::ios::app);

    if (csvdata.is_open()) {
        csvdata << std::fixed << std::setprecision(10);

        static bool headerWritten = false;
        if (!headerWritten) {
            csvdata << "Time,Vehicle_x,Vehicle_y,Speed,DataRate\n";
            headerWritten = true;
        }

        csvdata << time << ","
                << Vehicle_x << ","
                << Vehicle_y << ","
                << Speed << ","
                << DataRate << "\n";

        csvdata.close();
    } else {
        EV << "Error: Could not open CSV file!" << endl;
    }
}
void UESafetyApp::initialize(int stage)
{
    EV << "UEWarningAlertApp::initialize - stage " << stage << endl;
    cSimpleModule::initialize(stage);
    // avoid multiple initializations
    if (stage!=inet::INITSTAGE_APPLICATION_LAYER)
        return;

    log = par("logger").boolValue();
    if (stage == INITSTAGE_LOCAL)
       {
        //Instantiation of parameters

        mecApp_on = 0;
        nbRcvMsg = 0;


       }

    //retrieve parameters
    size_ = 1;
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

    DataRate_ = par("DataRate");
    //retrieving mobility module
    cModule *temp = getParentModule()->getSubmodule("mobility");
    if(temp != NULL){
        mobility = check_and_cast<inet::IMobility*>(temp);

        //L2S-ESME
       //inet::IMobility *mobility_ = check_and_cast<inet::IMobility *>(temp);
       //mobility_->getCurrentPosition();
       inet::Coord coord = getcoord(ue);
       double Speed = mobility->getMaxSpeed();
       double x = coord.x;
       double y = coord.y;

       SAFile.open("Vehicle.txt", std::ios::app);  // Open file for appending

        if (SAFile.is_open()) {
            SAFile << "Coordinates: ["<< coord.x << ";" << coord.y << "]" << std::endl;
            SAFile.close();
        }



        SAFile.open("Vinfo.txt", std::ios::app);  // Open file for appending

         if (SAFile.is_open()) {
             SAFile << " X: "<< coord.x << " Y :" << coord.y << " Speed: " << Speed << " DataRate: " << DataRate_ << std::endl;
             SAFile.close();
         }


        //Write data in the dataset
        writetoCSV(simTime(), x, y, Speed, DataRate_);

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
    selfSendData_ = new cMessage("selfSendData");

    UE_name_ = par("deviceAppAddress").stringValue();
    log= false;

    //maxCpuSpeed_ = par("maxCpuSpeed");
    //App_name_ = par("name").stringValue();

    //auto timestamp = std::time(nullptr);
    //static int counter = 0;

//    auto ev = getSimulation()->getActiveEnvir();
//    auto currentRun = ev->getConfigEx()->getActiveRunNumber();
//    auto log_identifier = "logs/"+to_string(currentRun)+"/";
      // Use the create_directory function to create the directory
//    if (mkdir(log_identifier.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0) {
//            std::cout << "Directory created successfully.\n";
//      } else {
//          std::cerr << "Error creating directory\n";
//      }


     payloadSize_ = par("payloadSize").doubleValue();

//     csv_filename_rcv_total = fmt::format("{}/ue_rcv_total.csv",log_identifier);
//     csv_filename_send_total = fmt::format("{}/ue_send_total.csv",log_identifier);
//
//     log_level = "low";
//     if (log_level=="high"){
//
//
//     ofstream myfile;
//
//     myfile.open (csv_filename_rcv_total, ios::app);
//     std::ifstream file(csv_filename_rcv_total);
//         if ( file.peek() == std::ifstream::traits_type::eof()) {
//       if(myfile.is_open())
//       {
//           myfile << "timestamp,idFrame,downlink_delay(s),deviceSimbolicAppAddress_,speed,roadID,ueAppPort,nbRcvMsg, from,runNumber" << endl;
//           myfile.close();
//       }
//       }
//
//
//     myfile.open (csv_filename_send_total, ios::app);
//     std::ifstream file2(csv_filename_send_total);
//         if ( file2.peek() == std::ifstream::traits_type::eof()) {
//       if(myfile.is_open())
//       {
//
//           myfile << "timestamp,interRequestTime(s),payloadsize(B),packetsize(B),deviceAppAddress_,car_sending,ueIP,speed,localPort_,idframe,runNumber" << endl;
//
//           myfile.close();
//       }
//         }
//
//     }
//     else
//      {
//     csv_filename_rcv_total = fmt::format("{}/ue_rcv_total.csv",log_identifier);
//
//     ofstream myfile;
//
//     myfile.open (csv_filename_rcv_total, ios::app);
//     std::ifstream file(csv_filename_rcv_total);
//         if ( file.peek() == std::ifstream::traits_type::eof()) {
//       if(myfile.is_open())
//       {
//           myfile << "timestamp,idFrame,downlink_delay(s),deviceSimbolicAppAddress_,speed,ueAppPort,nbRcvMsg, from,runNumber" << endl;
//           myfile.close();
//       }
//         }
//             myfile.open (csv_filename_send_total, ios::app);
//     std::ifstream file2(csv_filename_send_total);
//         if ( file2.peek() == std::ifstream::traits_type::eof()) {
//       if(myfile.is_open())
//       {
//
//            myfile << "timestamp,interRequestTime(s),payloadsize(B),packetsize(B),deviceAppAddress_,car_sending,ueIP,speed,localPort_,idframe,runNumber" << endl;
//
//           myfile.close();
//       }
//         }
//
//     }



    //starting UEWarningAlertApp
    simtime_t startTime = par("startTime");

    EV << "UEWarningAlertApp::initialize - starting sendStartMEWarningAlertApp() in " << startTime << " seconds " << endl;
    scheduleAt(simTime() + startTime, selfStart_);

    //testing
    EV << "UEWarningAlertApp::initialize - sourceAddress: " << sourceSimbolicAddress << " [" << inet::L3AddressResolver().resolve(sourceSimbolicAddress).str()  <<"]"<< endl;
    EV << "UEWarningAlertApp::initialize - destAddress: " << deviceSimbolicAppAddress_ << " [" << deviceAppAddress_.str()  <<"]"<< endl;
    EV << "UEWarningAlertApp::initialize - binding to port: local:" << localPort_ << " , dest:" << deviceAppPort_ << endl;

    DLPacketDelay_ = registerSignal("DLPacketDelay");
    packetRcvdMsg_ = registerSignal("packetRcvdMsg");
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

//double exponentialGen(double lambda) {
//    std::random_device rd;
//    std::mt19937 gen(rd());
//    std::exponential_distribution<> exponential(lambda);
//    double generated;
//    generated = exponential(gen);
//    return generated;
//}


void UESafetyApp::handleMessage(cMessage *msg)
{
    EV << "UEWarningAlertApp::handleMessage" << endl;
    // Sender Side
    if (msg->isSelfMessage())
    {
        if(!strcmp(msg->getName(), "selfStart")) {
            cancelEvent(selfSendData_);
            sendStartMEWarningAlertApp();
        }

        else if(!strcmp(msg->getName(), "selfStop"))    sendStopMEWarningAlertApp();

        else if(!strcmp(msg->getName(), "selfMecAppStart"))
        {
            scheduleAt(simTime() + period_, selfMecAppStart_);
        }


        else if(!strcmp(msg->getName(), "selfSendData")) {
            double time_to_wait_d ;
            //string name = par("name").stringValue();

            time_to_wait_d= exponentialGenerator(DataRate_);

            simtime_t time_to_wait {time_to_wait_d};

            //myLogger->info(fmt::format("UEPingPongApp::handleMessage- Ping sent at  {}s will wait {}s to send again",simTime().str(),time_to_wait.str()));
            //sendDataPacket(time_to_wait);
            sendDataPacket(time_to_wait);


            simtime_t  stopTime = par("stopTime");

            //if (stopTime - simTime()- time_to_wait > 0){
            cancelEvent(selfSendData_);
            scheduleAt(simTime() + time_to_wait, selfSendData_);
            //scheduleAt(simTime() + 15, selfSendData_);
        }

        else    throw cRuntimeError("UESafetyApp::handleMessage - \tWARNING: Unrecognized self message",msg->getName());



    }
    // Receiver Side
    else{

        try {
                       auto ret = dynamic_cast<inet::Packet*>(msg);
                       if (!ret){
                                   delete msg;
                                   return;

                               }


                       inet::Packet* packet = check_and_cast<inet::Packet*>(msg);

        inet::L3Address ipAdd = packet->getTag<L3AddressInd>()->getSrcAddress();
        int port = packet->getTag<L4PortInd>()->getSrcPort();

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

            if (!strcmp(msg->getName(), "DataPacket"))
            {

                auto mecPk = packet->peekAtFront<DataPacket>();
                auto pubPk = dynamicPtrCast<const DataPacket>(mecPk);


                // emit statistics
                simtime_t delay = simTime() - packet->getCreationTime();
                emit(DLPacketDelay_, delay);
                emit(packetRcvdMsg_, (long)1);
                nrReceived++;
                delaySum+=delay;

                EV <<"] DLPacketDelay[" << delay << "]" << endl;

                dFile.open("DLdelays.txt", std::ios::app);  // Open file for appending

                if (dFile.is_open()) {
                    dFile << " DLPacketDelay: "<< delay << std::endl;
                    dFile.close();
                }

            }
            //L2S-ESME
            else if (!strcmp(msg->getName(), "StartWarning"))
            {
                handleNotificationMessage(msg);
            }
            //L2S-ESME

        }
        delete msg;

                    } catch (const cRuntimeError& error){

                  delete msg;
                  return;
            } catch (const cRuntimeError error){

                  delete msg;
                  return;
            }
            catch (...) {

                  delete msg;
                  return;
            }


    }
}

void UESafetyApp::finish()
{

}
/*
 * -----------------------------------------------Sender Side------------------------------------------
 */
void UESafetyApp::sendStartMEWarningAlertApp()
{
    inet::Packet* packet = new inet::Packet("SafetyPacketStart");
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
    cancelEvent(selfStart_);
    //scheduleAt(simTime() + period_, selfStart_);
    EV << "UESafetyApp::START MECAPPPP 1111111111111111111111"<< endl;
}
void UESafetyApp::sendStopMEWarningAlertApp()
{
    EV << "UESafetyApp::sendStopMEWarningAlertApp - Sending " << STOP_MEAPP <<" type WarningAlertPacket\n";

    inet::Packet* packet = new inet::Packet("DeviceAppStopPacket");
    auto stop = inet::makeShared<DeviceAppStopPacket>();

    //termination requirements and info
    stop->setType(STOP_MECAPP);

    stop->setChunkLength(inet::B(size_));
    stop->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());

    packet->insertAtBack(stop);

    //cancelEvent(selfSendData_); //Test

    socket.sendTo(packet, deviceAppAddress_, deviceAppPort_);


    SAFile.open("mecapp.txt", std::ios::app);  // Open file for appending

    if (SAFile.is_open()) {
        SAFile << " Send Stop MEC App at: "<< simTime() << std::endl;
        SAFile.close();
    }

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
    mecApp_on = 0;
    emit( failed_to_start_mecApp_Signal,  mecApp_on );

}

/*
 * ---------------------------------------------Receiver Side------------------------------------------
 */
void UESafetyApp::handleAckStartMEWarningAlertApp(cMessage* msg)
{
    try{
        inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
        auto pkt = packet->peekAtFront<DeviceAppStartAckPacket>();

        auto ackt = inet::makeShared<DeviceAppStartPacket>();


        if(pkt->getResult() == true)
        {
            mecAppAddress_ = L3AddressResolver().resolve(pkt->getIpAddress());
            mecAppPort_ = pkt->getPort();
            EV << "UESafetyApp::handleAckStartMEWarningAlertApp - Received " << pkt->getType() << " type WarningAlertPacket. mecApp isntance is at: "<< mecAppAddress_<< ":" << mecAppPort_ << endl;
            cancelEvent(selfStart_);
            //scheduling sendStopMEWarningAlertApp()
            if(!selfStop_->isScheduled()){
                simtime_t  stopTime = par("stopTime");
                scheduleAt(simTime() + stopTime, selfStop_);

                SAFile.open("mecapp.txt", std::ios::app);  // Open file for appending

               if (SAFile.is_open()) {
                   SAFile << " Scheduling stop message at: "<< simTime() << std::endl;
                   SAFile.close();
               }
                EV << "UEWarningAlertApp::handleAckStartMEWarningAlertApp - Starting sendStopMEWarningAlertApp() in " << stopTime << " seconds " << endl;
            }

            // emit statistics
             simtime_t initdelay = simTime() - 2;
             emit(initDelay_, initdelay);
             // emit(alertRcvdMsg_, (long)1);
             initnrReceived++;
             initdelaySum+=initdelay;

             EV <<"] initDelay[" << initdelay << "]" << endl;

             // Send circle notification subscription to MEC Warning App
             cancelEvent(selfSendData_);//Teeeessstttt
             sendCircleInformations(); //L2S-ESME
             scheduleAt(simTime() + period_, selfSendData_);

             //Start send Data
             //cancelEvent(selfSendData_);
            // scheduleAt(simTime(), selfSendData_);
            // mecApp_on = 1;
            // emit( failed_to_start_mecApp_Signal,  mecApp_on );


             //sendMessageToMECApp();
             //scheduleAt(simTime() + period_, selfMecAppStart_);


        }

        else
        {
            EV << "UESafetyApp::handleAckStartMEWarningAlertApp - MEC application cannot be instantiated! Reason: " << pkt->getReason() << endl;

            if ( !strcmp(pkt->getReason(), "LCM proxy responded 500")){

                nb_fails+=1;

                mecApp_on= 0;
                emit( failed_to_start_mecApp_Signal,  mecApp_on );

                if (nb_fails>2){throw cRuntimeError("test"); }
            }

        }


    }
    catch (const cRuntimeError& error){

    delete msg;
    return;
        }
           catch (const cRuntimeError error){

    delete msg;
    return;
        }


    //sendMessageToMECApp();
    //scheduleAt(simTime() + period_, selfMecAppStart_);

}



//void UEWarningAlertApp::sendMessageToMECApp(){
//
//    // send star monitoring message to the MEC application
//
//    inet::Packet* pkt = new inet::Packet("WarningAlertPacketStart");
//    auto alert = inet::makeShared<WarningStartPacket>();
//    alert->setType(START_WARNING);
//    alert->setCenterPositionX(par("positionX").doubleValue());
//    alert->setCenterPositionY(par("positionY").doubleValue());
//    alert->setRadius(par("radius").doubleValue());
//
//
//    int payload;
//    payload = exponentialGen(1.0 / 40000);
//    payload = (payload>65527)? 65527:payload;
//    payload = (payload<1)? 1:payload;
//    EV<<"RRRRRRRRRRRRRRRRRRRRRRRRRRR"<<payload<<endl;
//
//    alert->setChunkLength(inet::B(payload));
//    // I add this line to read the start time
//    alert->setPayloadTimestamp(simTime());
//    alert->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
//    pkt->insertAtBack(alert);
//
//    if(log)
//    {
//        ofstream myfile;
//        myfile.open ("example.txt", ios::app);
//        if(myfile.is_open())
//        {
//            myfile <<"["<< NOW << "] UEWarningAlertApp - UE sent start subscription message to the MEC application \n";
//            myfile.close();
//        }
//    }
//
//    socket.sendTo(pkt, mecAppAddress_ , mecAppPort_);
//    EV << "UEWarningAlertApp::sendMessageToMECApp() - start Message sent to the MEC app" << endl;
//}

void UESafetyApp::sendDataPacket(simtime_t interReqTime) {
    //
    // sending Data to MEC application
    inet::Packet* pkt2 = new inet::Packet("DataPacket");
    auto alert2 = inet::makeShared<DataPacket>();
    alert2->setType(START_PINGPONG);
    alert2->setdata("DATA");

    //iDframe_ = ++iDframe_;
    static unsigned int  iDframe_ = 0;
    iDframe_++;
    alert2->setIDframe(iDframe_);

    //string name = par("name").stringValue();
    int payload;
    payload = exponentialGenerator(1.0 / 40000);

    payload = (payload>65527)? 65527:payload;
    payload = (payload<1)? 1:payload;
    alert2->setChunkLength(inet::B(payload));
    ue->bubble("sending");
    alert2->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
    pkt2->insertAtBack(alert2);

//    auto ev = getSimulation()->getActiveEnvir();
//    auto currentRun = ev->getConfigEx()->getActiveRunNumber();
//
//    ofstream myfile;
//    myfile.open (csv_filename_send_total, ios::app);
//    if(myfile.is_open())
//       {
//           //myfile << "timestamp,interRequestTime(s),payloadsize(B),packetsize(B),deviceAppAddress_,deviceSimbolicAppAddress_,ueIP,speed,ueAppport" << endl;
//           myfile << simTime() <<","<< interReqTime << "," << payload<< ","<<  pkt2->getByteLength() <<"," <<deviceAppAddress_<<"," <<deviceSimbolicAppAddress_ << "," << inet::L3AddressResolver().resolve(sourceSimbolicAddress) << ","<< par("localPort").intValue()<< "," << iDframe_ <<"," <<"currentPosition" <<","<<currentRun << endl;
//           myfile.close();
//       }
//

    socket.sendTo(pkt2, mecAppAddress_, mecAppPort_);

    //myLogger->info("@ Sent ping to the MEC");
}


//L2S-ESME
void UESafetyApp::sendCircleInformations(){

    // Creating a packet to send the circle notification subscription request to the MEC application
    inet::Packet* pkt = new inet::Packet("StartWarning");
    auto alert = inet::makeShared<DataPacket>();

    // Set the type of subscription packet
    alert->setType(START_WARNING);
    alert->setCenterPositionX(par("positionX").doubleValue());
    alert->setCenterPositionY(par("positionY").doubleValue());
    alert->setRadius(par("radius").doubleValue());
    alert->setPayloadTimestamp(simTime());
    // Generate a random payload size, keeping within valid bounds
    //int payload = exponentialGen(1.0 / 40000);
    //payload = (payload > 65527) ? 65527 : payload;
    //payload = (payload < 1) ? 1 : payload;

    alert->setChunkLength(inet::B(5));

    // Record the current simulation time as the payload timestamp
    alert->setPayloadTimestamp(simTime());
    alert->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());

    pkt->insertAtBack(alert);

    // Send the packet to the MEC application
    socket.sendTo(pkt, mecAppAddress_, mecAppPort_);

    // Logging to a file that the subscription message has been sent
    SAFile.open("output.txt", std::ios::app);  // Open file for appending
    if (SAFile.is_open())
    {
        SAFile << "UESafetyApp::sendCircleNotificationSubscription() - Subscription Message sent to the MEC app. Address: "
               << mecAppAddress_ << ", Port: " << mecAppPort_ << std::endl;

        SAFile.close();
    }

    SAFile.open("migration.txt", std::ios::app);  // Open file for appending
        if (SAFile.is_open())
        {
            SAFile << "UESafetyApp::Received ACK and send to MEC at: "<< simTime() << std::endl;

            SAFile.close();
        }

}

//L2S-ESME

//L2S-ESME
void UESafetyApp::handleNotificationMessage(cMessage* msg)
{
    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    auto pkt = packet->peekAtFront<SafetyPacket>();

    EV << "UEWarningAlertApp::handleInfoMEWarningrAlertApp - Received " << pkt->getType() << " type WarningAlertPacket"<< endl;


    //updating runtime color of the car icon background
    if(pkt->getDanger())
    {
        // emit statistics
        /*simtime_t delay = simTime() - pkt->getPayloadTimestamp();
        emit(alertMECDelay_, delay);
        emit(alertRcvdMsg_, (long)1);
        nrReceived++;
        delaySum+=delay;*/


        EV << "UEWarningAlertApp::handleInfoMEWarningrAlertApp - Warning Alert Detected: DANGER!" << endl;
        ue->getDisplayString().setTagArg("i",1, "red");
    }
    else{

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


//L2S-ESME
//Not used in this app
void UESafetyApp::handleInfoMEWarningAlertApp(cMessage* msg)
{
    try {
        inet::Packet* packet = check_and_cast<Packet *>(msg);
        auto pkt = packet->peekAtFront<DataPacket>();

        EV << "UESafetyApp::handleInfoMEWarningrAlertApp - Received " << pkt->getType() << " type WarningAlertPacket"<< endl;

        //updating runtime color of the car icon background

            if(log)
            {
                ofstream myfile;
                myfile.open ("example.txt", ios::app);
                if(myfile.is_open())
                {
                    myfile <<"["<< NOW << "] UEPingPongApp - UE received danger alert FALSE from MEC application \n";
                    myfile.close();
                }
            }

            EV << "UESafetyApp::handleInfoMEWarningrAlertApp - Warning Alert Detected: NO DANGER!" << endl;
            //ue->getDisplayString().setTagArg("i",1, "green");

            }
                  catch (const cRuntimeError& error){
                                        //myLogger->info(fmt::format("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));

                      //myLogger->info(fmt::format("ERROR {}///{}///{}///{}///{}///{}///{}///",error.what(),error.what(),error.what(),error.what(),error.what(),error.what(),error.what()));
                      delete msg;
            }

}
void UESafetyApp::handleAckStopMEWarningAlertApp(cMessage* msg)
{

    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    auto pkt = packet->peekAtFront<DeviceAppStopAckPacket>();

    EV << "UEWarningAlertApp::handleAckStopMEWarningAlertApp - Received " << pkt->getType() << " type WarningAlertPacket with result: "<< pkt->getResult() << endl;
    if(pkt->getResult() == false)
        EV << "Reason: "<< pkt->getReason() << endl;
    //updating runtime color of the car icon background
    ue->getDisplayString().setTagArg("i",1, "white");

    mecApp_on = 0;
    emit( failed_to_start_mecApp_Signal,  mecApp_on );

    cancelEvent(selfStop_);

}
