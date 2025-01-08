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

#include "nodes/mec/MECOrchestrator/mecHostSelectionPolicies/BestSelectionBased.h"
#include "nodes/mec/MECPlatformManager/MecPlatformManager.h"
#include "nodes/mec/VirtualisationInfrastructureManager/VirtualisationInfrastructureManager.h"
#include <chrono>
#include <thread>
#include <future>
#include <vector>
#include <cmath>

#include <fstream>
#include <iostream>
#include <iomanip>
#include "UseFunctions.h"

std::ofstream RFile;
std::ofstream UTFile;

// Function to calculate Euclidean distance between two points
inline double calculateDistance(double x1, double y1, double x2, double y2)
{
    return std::sqrt(std::pow(x2 - x1, 2) + std::pow(y2 - y1, 2));
}

cModule* BestSelectionBased::findBestMecHost(const ApplicationDescriptor& appDesc)
{
    auto start = std::chrono::high_resolution_clock::now();
    cModule* bestHost = nullptr;
    double maxCompositeScore = -1;
    std::vector<std::pair<cModule*, double>> bestHosts;

    for (auto mecHost : mecOrchestrator_->mecHosts)
    {
        VirtualisationInfrastructureManager* vim = check_and_cast<VirtualisationInfrastructureManager*>(mecHost->getSubmodule("vim"));
        ResourceDescriptor resources = appDesc.getVirtualResources();
        bool res = vim->isAllocable(resources.ram, resources.disk, resources.cpu);
        vim->PrintResources();
        if (!res)
        {
            continue;
        }

        MecPlatformManager* mecpm = check_and_cast<MecPlatformManager*>(mecHost->getSubmodule("mecPlatformManager"));
        auto mecServices = mecpm->getAvailableMecServices();
        std::string serviceName;

        if (appDesc.getAppServicesRequired().size() > 0)
        {
            serviceName = appDesc.getAppServicesRequired()[0];
        }
        else
        {
            return mecHost;
        }

        for (const auto& mecService : *mecServices)
        {
            if (serviceName.compare(mecService.getName()) == 0 && mecService.getMecHost().compare(mecHost->getName()) == 0)
            {
                double compositeScore = 0.5 * vim->getAvailableResources().cpu + 0.3 * vim->getAvailableResources().ram + 0.2 * vim->getAvailableResources().disk;
                RFile.open("Load.txt", std::ios::app);  // Open file for appending

                if (RFile.is_open()) {
                    RFile << std::fixed << std::setprecision(10);  // Set fixed notation with 2 decimal places
                    RFile << "The load of mecHost:  " << mecHost->getName() << " is " << compositeScore << std::endl;
                    RFile.close();
                }
                bestHosts.push_back({mecHost, compositeScore});
                break;
            }
        }
    }

    if (bestHosts.size() < 1)
       {
           return nullptr;
       }

    if (bestHosts.size() >= 2){
        // Sort the bestHosts by composite score in descending order and keep the top two
        std::sort(bestHosts.begin(), bestHosts.end(), [](const auto& a, const auto& b) {
            return a.second > b.second;
        });
        bestHosts.resize(2);

        // Step 2: Select the closest MEC host among the top two based on distance to the vehicle
        // Read Base Stations from BS.txt
        std::ifstream bsFile("BS.txt");
        std::vector<std::pair<std::string, std::pair<double, double>>> baseStations;
        std::string line;
        while (std::getline(bsFile, line))
        {
            size_t pos = line.find(":");
            std::string bsName = line.substr(0, pos);
            size_t coordStart = line.find("[");
            if (coordStart != std::string::npos)
            {
                size_t coordEnd = line.find("]");
                std::string coords = line.substr(coordStart + 1, coordEnd - coordStart - 1);
                size_t sep = coords.find(";");
                double x = std::stod(coords.substr(0, sep));
                double y = std::stod(coords.substr(sep + 1));
                baseStations.push_back({bsName, {x, y}});
            }
        }
        bsFile.close();

        // Read Vehicle coordinates from Vehicle.txt
        std::ifstream vehicleFile("Vehicle.txt");
        double vehicleX, vehicleY;
        if (std::getline(vehicleFile, line))
        {
            size_t pos = line.find("[");
            if (pos != std::string::npos)
            {
                size_t endPos = line.find("]");
                std::string coords = line.substr(pos + 1, endPos - pos - 1);
                size_t sep = coords.find(";");
                vehicleX = std::stod(coords.substr(0, sep));
                vehicleY = std::stod(coords.substr(sep + 1));
            }
        }
        vehicleFile.close();

        // Calculate distance between the vehicle and the base stations corresponding to the selected MEC hosts
        double minDistance = std::numeric_limits<double>::max();
        cModule* closestMecHost = nullptr;
        for (const auto& hostPair : bestHosts)
        {
            cModule* mecHost = hostPair.first;
            std::string mecHostName = mecHost->getName();
            for (const auto& baseStation : baseStations)
            {
                if (mecHostName == "mecHost" + baseStation.first.substr(2))
                {
                    double bsX = baseStation.second.first;
                    double bsY = baseStation.second.second;
                    double distance = calculateDistance(vehicleX, vehicleY, bsX, bsY);
                    if (distance < minDistance)
                    {
                        minDistance = distance;
                        closestMecHost = mecHost;

                        RFile.open("Score.txt", std::ios::app);  // Open file for appending

                        if (RFile.is_open()) {
                            RFile << "The mecHost : " << closestMecHost << " with distance: "<< distance << std::endl;
                            RFile.close();
                        }
                    }
                    break;
                }
            }
        }

        // Delete first row from Vehicle.txt
        std::ifstream inFile("Vehicle.txt");
        std::ofstream tempFile("Vehicle_temp.txt");
        bool isFirstLine = true;
        while (std::getline(inFile, line))
        {
            if (isFirstLine)
            {
                isFirstLine = false;
                continue;
            }
            tempFile << line << std::endl;
        }
        inFile.close();
        tempFile.close();
        std::remove("Vehicle.txt");
        std::rename("Vehicle_temp.txt", "Vehicle.txt");

        // Log the minimum distance
        RFile.open("Score.txt", std::ios::app);  // Open file for appending
        if (RFile.is_open()) {
            RFile << "The minimum distance between vehicle and MEC host is : " << minDistance << " units" << std::endl;
            RFile.close();
        }

        auto end = std::chrono::high_resolution_clock::now();
        long duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        RFile.open("Score.txt", std::ios::app);  // Open file for appending
        if (RFile.is_open()) {
            RFile << "The relocation duration is : " << duration << "ms" << std::endl;
            RFile.close();
        }

        RFile.open("Score.txt", std::ios::app);  // Open file for appending

        if (RFile.is_open()) {
            RFile << "The closest MEC Host is : " << closestMecHost << std::endl;
            RFile.close();
        }
        ResourceDescriptor resources = appDesc.getVirtualResources();
        UTFile.open("Util.txt", std::ios::app);  // Open file for appending
        if (UTFile.is_open()) {
            UTFile << "The : " << closestMecHost << " , "<< " CPU "  << resources.cpu << std::endl;
            UTFile << "The : " << closestMecHost << " , "<< " RAM "  << resources.ram << std::endl;
            UTFile << "The : " << closestMecHost << " , "<< " Disk "  << resources.disk << std::endl;
            UTFile.close();
        }
        //VirtualisationInfrastructureManager* vim = check_and_cast<VirtualisationInfrastructureManager*>(closestMecHost->getSubmodule("vim"));
       // vim->PrintResources();
        return closestMecHost;  // Return the closest MEC host
    }
    else{
        for (const auto& W : bestHosts)
               {
                    cModule* mecHost = W.first;
                    return mecHost;
               }

    }

   }
