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

#include "nodes/mec/MECPlatformManager/MecPlatformManager.h"
#include "nodes/mec/MECOrchestrator/MecOrchestrator.h"
#include "nodes/mec/MECPlatform/MECServices/LocationService/LocationService.h"
#include "nodes/mec/MECOrchestrator/MECOMessages/MECOrchestratorMessages_m.h"
#include <fstream>

Define_Module(MecPlatformManager);

std::ofstream PFile;

MecPlatformManager::MecPlatformManager()
{
    vim = nullptr;
    serviceRegistry = nullptr;
    mecOrchestrator = nullptr;
    locationService = nullptr;
}

void MecPlatformManager::initialize(int stage)
{
    EV << "VirtualisationInfrastructureManager::initialize - stage " << stage << endl;
    cSimpleModule::initialize(stage);
    // avoid multiple initializations
    if (stage!=inet::INITSTAGE_LOCAL)
        return;
    vim = check_and_cast<VirtualisationInfrastructureManager*>(getParentModule()->getSubmodule("vim"));
    cModule* mecPlatform = getParentModule()->getSubmodule("mecPlatform");
    if(mecPlatform->findSubmodule("serviceRegistry") != -1)
    {
        serviceRegistry = check_and_cast<ServiceRegistry*>(mecPlatform->getSubmodule("serviceRegistry"));
        /*PFile.open("output.txt", std::ios::app);

       if (PFile.is_open()) {
           PFile << "The service Registry in the mecPlatform manager:"<< mecPlatform->getSubmodule("serviceRegistry") << std::endl;

           PFile.close();
       }*/

       cModule* Lservice = mecPlatform->getSubmodule("LocationService");
             /*PFile.open("output.txt", std::ios::app);

            if (PFile.is_open()) {
                PFile << "The location service in the mecPlatform manager:"<< Lservice << std::endl;

                PFile.close();
            }*/
    }
     //L2S-ESME
    //cModule* mecPlatform = getParentModule()->getSubmodule("mecPlatform");



    if(mecPlatform->findSubmodule("LocationService") != -1)
    {
        locationService = check_and_cast<LocationService*>(mecPlatform->getSubmodule("LocationService"));

    }
    //L2S-ESME
    const char * mecOrche = par("mecOrchestrator").stringValue();
    cModule* module = getSimulation()->findModuleByPath(mecOrche);

    if(module != nullptr)
        mecOrchestrator = check_and_cast<MecOrchestrator*>(module);
    else
    {
        EV << "MecPlatformManager::initialize - Mec Orchestrator ["<< mecOrche << "] not found" << endl;
    }
}


// instancing the requested MEApp (called by handleResource)
MecAppInstanceInfo* MecPlatformManager::instantiateMEApp(CreateAppMessage* msg)
{
   MecAppInstanceInfo* res = vim->instantiateMEApp(msg);
   delete msg;
   return res;
}

bool MecPlatformManager::instantiateEmulatedMEApp(CreateAppMessage* msg)
{
    bool res = vim->instantiateEmulatedMEApp(msg);
    delete msg;
    return res;
}

bool MecPlatformManager::terminateEmulatedMEApp(DeleteAppMessage* msg)
{
    bool res = vim->terminateEmulatedMEApp(msg);
    delete msg;
    return res;
}


// terminating the correspondent MEApp (called by handleResource)
bool MecPlatformManager::terminateMEApp(DeleteAppMessage* msg)
{
    bool res = vim->terminateMEApp(msg);
    delete msg;
    return res;
}

//L2S-ESME
bool MecPlatformManager::terminateR(DeleteAppMessage* msg ,cModule* mecHost)
{
    bool res = vim->terminateR(msg, mecHost);
    delete msg;
    return res;

}
//L2S-ESME
const std::vector<ServiceInfo>* MecPlatformManager::getAvailableMecServices() const
{
    if(serviceRegistry == nullptr)
        return nullptr;
    else
    {
       return serviceRegistry->getAvailableMecServices();

    }
}

void MecPlatformManager::registerMecService(ServiceDescriptor& serviceDescriptor) const
{
    if(mecOrchestrator != nullptr){
        mecOrchestrator->registerMecService(serviceDescriptor);
        EV << "ServiceRegistry::Rim11 - Hela11" << endl;
    }

}

//L2S-ESME
void MecPlatformManager::sendMigrationNotification(int vehicleId) const
{
    if(mecOrchestrator != nullptr){
        mecOrchestrator->sendMigrationNotification(vehicleId);

        PFile.open("output.txt", std::ios::app);  // Open file for appending

        if (PFile.is_open()) {
            PFile << "The vehicle Id successfully received by the mecPlatformManager, it is : "<< vehicleId << std::endl;
            PFile.close();
        }

    }

}




