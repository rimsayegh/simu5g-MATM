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

#include "nodes/mec/MECOrchestrator/mecHostSelectionPolicies/AvailableResourcesSelectionBased.h"
#include "nodes/mec/MECPlatformManager/MecPlatformManager.h"
#include "nodes/mec/VirtualisationInfrastructureManager/VirtualisationInfrastructureManager.h"
#include <chrono>



#include <fstream>
#include <iostream>

std::ofstream FFile;
std::ofstream UAFile;


cModule* AvailableResourcesSelectionBased::findBestMecHost(const ApplicationDescriptor& appDesc)
{
    auto start = std::chrono::high_resolution_clock::now();
    //EV<<"start:"<<start<<endl;

    EV << "AvailableResourcesSelectionBased::findBestMecHost - finding best MecHost..." << endl;
    cModule* bestHost = nullptr;
    double maxCpuSpeed = -1;

    for(auto mecHost : mecOrchestrator_->mecHosts)
    {
       VirtualisationInfrastructureManager *vim = check_and_cast<VirtualisationInfrastructureManager*> (mecHost->getSubmodule("vim"));
       ResourceDescriptor resources = appDesc.getVirtualResources();
       bool res = vim->isAllocable(resources.ram, resources.disk, resources.cpu);
       vim->PrintResources();
       if(!res)
       {
           EV << "AvailableResourcesSelectionBased::findBestMecHost - MEC host ["<< mecHost->getName() << "] has not got enough resources. Searching again..." << endl;
           continue;
       }
       if(vim->getAvailableResources().cpu > maxCpuSpeed)
       {
           // Temporally select this mec host as the best
           EV << "AvailableResourcesSelectionBased::findBestMecHost - MEC host ["<< mecHost->getName() << "] temporally chosen as bet MEC host. Available resources: " << endl;
           vim->printResources();
           bestHost = mecHost;
           maxCpuSpeed = vim->getAvailableResources().cpu;

           //test
           double compositeScore = 0.5 * vim->getAvailableResources().cpu + 0.3 * vim->getAvailableResources().ram + 0.2 * vim->getAvailableResources().disk;
           FFile.open("Load.txt", std::ios::app);  // Open file for appending

           if (FFile.is_open()) {
               FFile << "The load of mecHost:  " << mecHost->getName() << " is " <<  compositeScore  << std::endl;
               FFile << "The mecHost: " << mecHost << std::endl;
               FFile << "The cpu available:  " << vim->getAvailableResources().cpu << std::endl;
               FFile << "The ram available:  " << vim->getAvailableResources().ram << std::endl;
               FFile << "The disk available:  " << vim->getAvailableResources().disk << std::endl;
               FFile.close();
           }
           //Test
       }
    }
    if(bestHost != nullptr)
        EV << "AvailableResourcesSelectionBased::findBestMecHost - MEC host ["<< bestHost->getName() << "] has been chosen as the best Mec Host" << endl;
    else
        EV << "AvailableResourcesSelectionBased::findBestMecHost - No Mec Host found" << endl;

    auto end = std::chrono::high_resolution_clock::now();
    long duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

    //EV<<"end:"<<end<<endl;
    EV<< "Execution time: " << duration << " milliseconds" << endl;

    ResourceDescriptor resources = appDesc.getVirtualResources();
    UAFile.open("Util.txt", std::ios::app);  // Open file for appending
    if (UAFile.is_open()) {
        UAFile << "The : " << bestHost << " , "<< " CPU "  << resources.cpu << std::endl;
        UAFile << "The : " << bestHost << " , "<< " RAM "  << resources.ram << std::endl;
        UAFile << "The : " << bestHost << " , "<< " Disk "  << resources.disk << std::endl;
        UAFile.close();
    }
    //VirtualisationInfrastructureManager* vim = check_and_cast<VirtualisationInfrastructureManager*>(bestHost->getSubmodule("vim"));
    //vim->PrintResources();
    return bestHost;
}

