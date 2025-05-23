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

package simu5g.nodes.mec.MECOrchestrator;

// 
// This module implements the MEC orchestrator of a MEC system.
// It does not follow ETSI compliant APIs, but the it handles the lifecycle operations 
// of the standard by using OMNeT++ features.
// Communications with the LCM proxy occur via connections, while the MEC hosts associated with
// the MEC system (and the MEC orchestrator) are managed with the mecHostList parameter.
// This MEC orchestrator provides:
//   - MEC app instantiation
//   - MEC app termination
//   - MEC app run-time onboarding
// 

simple MecOrchestrator
{
    parameters:
        @display("i=device/mainframe;bgb=1006.50995,692.76");
        string selectionPolicy = default("MecServiceBased"); // available policies: "MecHostBased", "MecServiceBased", "AvailableResourcesBased", "BestHostBased", "DLBM"
       	int mecHostIndex = default(0); // to be used with the MecHostBased policybased
	    string mecHostList = default("");
	    string mecApplicationPackageList = default("");
	    
	    //L2S-ESME 
	    double alpha = default(1);
	    double betta = default(1);
	    double lamda = default(1.5);
	    // times to simulate lifecycle operations 
	    double onboardingTime @unit(s) = default(2ms);
	    double instantiationTime @unit(s) = default(2ms);
	    double terminationTime @unit(s) = default(2ms);
	
    gates:
                
        output toUALCMP;
        input fromUALCMP;
        
        //input fromLS; //gate to receive message from LocationService    //L2S-ESME
        //output toLS;  //gate to send message to LocationService    //L2S-ESME

}