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


import inet.common.INETDefs;
import inet.common.packet.chunk.Chunk;

import nodes.mec.MECPlatform.MECPackets;
//cplusplus {{
//#include "nodes/mec/MECPlatform/MECPackets_m.h";
//}}
//import lte.nodes.mec.MEPlatform.MECPackets.MECPacket;


class DeviceAppPacket extends MECPacket {
//    unsigned int sno;
//    simtime_t timestamp;
//    
//    //type of message
//    string type;
//    string operation;
}

class DeviceAppStartPacket extends DeviceAppPacket
{
    string mecAppDId;
	string mecAppName;
	string mecAppProvider;
	string appPackageSource;
	simtime_t payloadTimestamp;
}


class DeviceAppStopPacket extends DeviceAppPacket
{
    string contextId;
}

class DeviceAppStartAckPacket extends DeviceAppPacket
{
    string contextId;
    bool result;
    string ipAddress;
    string reason;
	int port;
	simtime_t payloadTimestamp;
}

class DeviceAppStopAckPacket extends DeviceAppPacket
{
    bool result;
    string reason;
}




