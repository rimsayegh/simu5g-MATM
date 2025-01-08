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


//
// IceAlert Packet used by UEIceAlertApp MEIceAlertApp and MEIceAlertService
//

import nodes.mec.MECPlatform.MECPackets;

class SafetyAppPacket extends MECPacket
{}

class SafetyPacket extends SafetyAppPacket
{       
    int ueOmnetID; 
    double positionX;
    double positionY;
    double positionZ;
    bool danger;
    simtime_t payloadTimestamp;
}

class SafetyStartPacket extends SafetyAppPacket
{       
	double centerPositionX;
	double centerPositionY;
	double centerPositionZ;
	simtime_t payloadTimestamp;
	double radius;
}

class SafetyStopPacket extends SafetyAppPacket
{       

}

class DataPacket extends MECPacket
{
    unsigned int IDframe;
    int ueOmnetID;
    string data;
    
    double centerPositionX;
	double centerPositionY;
	double centerPositionZ;
	simtime_t payloadTimestamp;
	double radius;
}
