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

package simu5g.apps.mec.Safety;

import inet.applications.contract.IApp;
import simu5g.nodes.mec.MECPlatform.IMECApp;

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


simple MECSafetyApp like IMECApp, IApp
{
    parameters:
        @display("i=block/app;is=s");
  		@signal[ULPacketDelay];
        @statistic[ULPacketDelay](title="UL Message Delay"; unit="s"; source="ULPacketDelay"; record=mean,vector);  	
        @signal[ULpacketRcvdMsg]; 
        @statistic[ULpacketRcvdMsg](title=" UL Packet Messages Received"; unit="s"; source="ULpacketRcvdMsg"; record=sum,vector); 
         
        @signal[QueueingMECDelay];
        @statistic[QueueingMECDelay](title="Queueing Delay"; unit="s"; source="QueueingMECDelay"; record=mean,vector);  	
//        @signal[ULpacketRcvdMsg]; 
//        @statistic[ULpacketRcvdMsg](title=" UL Packet Messages Received"; unit="s"; source="ULpacketRcvdMsg"; record=sum,vector); 
        @signal[processingMECDelay];
        @statistic[processingMECDelay](title="calculate the processing Time in the MEC App"; unit="s"; source="processingMECDelay"; record=mean,vector);  	
        
         
        @signal[processingDelay];
        @statistic[processingDelay](title="calculate the processing Time in the MEC"; unit="s"; source="processingDelay"; record=mean,vector);  	
        
        int packetSize = default(100);
        string interfaceTableModule = default("");
        
        int mecAppIndex;
        int mecAppId;
        int localUePort;
        
        string mp1Address = default("");
        int mp1Port = default(10021);
        
		 //resource required info
        double requiredRam @unit("B");
        double requiredDisk @unit("B");
        double requiredCpu;
        double radius;

		// IApp parameters
		int timeToLive = default(-1); // if not -1, set the TTL (IPv4) or Hop Limit (IPv6) field of sent packets to this value
        int dscp = default(-1); // if not -1, set the DSCP (IPv4/IPv6) field of sent packets to this value
        int tos = default(-1); // if not -1, set the Type Of Service (IPv4) / Traffic Class (IPv6) field of sent packets to this value
		
		bool logger = default(false);
		
	    //double maxCpuSpeed;
		//string name;
		@signal[mecapp_ul_delaySignal];
		@statistic[mecappul_delay](title="e2e delay"; unit="notyet"; source="mecapp_ul_delaySignal"; record=timeavg,mean,vector,histogram,"last(sumPerDuration)",sum);
		@signal[mecapp_pk_rcv_sizeSignal];
		@statistic[mecapp_pk_rcv_size](title="size of preceived packet"; unit="notyet"; source="mecapp_pk_rcv_sizeSignal"; record=timeavg,mean,vector,histogram,"last(sumPerDuration)",sum);
		

    gates:
	  input mePlatformIn;
	  output mePlatformOut;
	  
	  input socketIn;
	  output socketOut;
                
}

