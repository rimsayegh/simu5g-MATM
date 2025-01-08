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

#include "MecHostSelectionBased.h"
#include "nodes/mec/MECPlatformManager/MecPlatformManager.h"
#include "nodes/mec/VirtualisationInfrastructureManager/VirtualisationInfrastructureManager.h"

#include <fstream>
#include <iostream>

std::ofstream UMFile;

MecHostSelectionBased::MecHostSelectionBased(MecOrchestrator* mecOrchestrator, int index):SelectionPolicyBase(mecOrchestrator)
{
    mecHostIndex_ = index;
}

cModule* MecHostSelectionBased::findBestMecHost(const ApplicationDescriptor& appDesc)
{
    EV << "MecHostSelectionBased::findBestMecHost - finding best MecHost..." << endl;
    cModule* bestHost = nullptr;

    int size = mecOrchestrator_->mecHosts.size();
    if(size < mecHostIndex_)
    {
        EV << "MecHostSelectionBased::findBestMecHost - No Mec Host with index [" << mecHostIndex_ << "] found" << endl;
    }
    else
    {
        ResourceDescriptor resources = appDesc.getVirtualResources();
        bestHost = mecOrchestrator_->mecHosts.at(mecHostIndex_);
        EV << "MecHostSelectionBased::findBestMecHost - MEC host ["<< bestHost->getName() << "] has been chosen as the best Mec Host" << endl;

        UMFile.open("Util.txt", std::ios::app);  // Open file for appending
        if (UMFile.is_open()) {
            UMFile << "The : " << bestHost << " , "<< " CPU "  << resources.cpu << std::endl;
            UMFile << "The : " << bestHost << " , "<< " RAM "  << resources.ram << std::endl;
            UMFile << "The : " << bestHost << " , "<< " Disk "  << resources.disk << std::endl;
            UMFile.close();
        }
    }
    VirtualisationInfrastructureManager* vim = check_and_cast<VirtualisationInfrastructureManager*>(bestHost->getSubmodule("vim"));
    vim->PrintResources();
    return bestHost;
}

