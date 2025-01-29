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

#include "nodes/mec/MECOrchestrator/mecHostSelectionPolicies/LoadAware.h"
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

#include <sstream>
#include <vector>
#include <map>
#include <algorithm>
#include <string>
#include <limits>


std::ofstream RsFile;
std::ofstream UTsFile;
std::ofstream Track;

std::vector<double> pastLBDs; // Store the past LBD values globally

double calculateLoadBalancingDegree(const std::vector<std::pair<cModule*, double>>& bestHosts) {
    int K = bestHosts.size();
    if (K == 0) return 0.0;  // Avoid division by zero

    // Step 1: Compute the mean Load
    double sumLoad = 0.0;
    for (const auto& entry : bestHosts) {
        sumLoad += entry.second;
    }
    double meanLoad = sumLoad / K;

    // Step 2: Compute the variance (LBD formula)
    double variance = 0.0;
    for (const auto& entry : bestHosts) {
        variance += pow(entry.second - meanLoad, 2);
    }
    double loadBalancingDegree = variance / K;

    return loadBalancingDegree;
}

double calculateMean(const std::vector<double>& values) {
    if (values.empty()) return 0.0;
    return std::accumulate(values.begin(), values.end(), 0.0) / values.size();
}

double calculateStdDev(const std::vector<double>& values, double mean) {
    if (values.size() < 2) return 0.0;
    double variance = 0.0;
    for (double val : values) {
        variance += pow(val - mean, 2);
    }
    return sqrt(variance / values.size());
}

inline double calculateDistance(double x1, double y1, double x2, double y2)
{
    return std::sqrt(std::pow(x2 - x1, 2) + std::pow(y2 - y1, 2));
}

cModule* LoadAware::findBestMecHost(const ApplicationDescriptor& appDesc)
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

                double load = vim->CalculateLoad();
                bestHosts.push_back({mecHost, load});

                RsFile.open("Load.txt", std::ios::app);  // Open file for appending

                if (RsFile.is_open()) {
                    RsFile << std::fixed << std::setprecision(10);  // Ensures 10 decimal places
                    RsFile << "Time:" << simTime() << " / " <<"The mecHost: " << mecHost->getFullPath() << " has load: " << load << std::endl;
                    RsFile.close();
                }

                break;
            }
        }
    }
    //Calculate the Load Balance degree
    double LBD = calculateLoadBalancingDegree(bestHosts);

    // Store the new LBD in the history
    pastLBDs.push_back(LBD);


    double meanLBD = calculateMean(pastLBDs);
    double stdDevLBD = calculateStdDev(pastLBDs, meanLBD);

    //sensitivity factor
    double lambda = 2;  //adjust this based on testing

    // Compute the dynamic threshold for LBD
    double LBDThreshold = meanLBD + lambda * stdDevLBD;
    //double LBDThreshold = meanLBD;

    RsFile.open("LBD.txt", std::ios::app);

    if (RsFile.is_open()) {

        RsFile << std::fixed << std::setprecision(10);  // Ensures 10 decimal places
        RsFile << "Time:" << simTime() << " / " <<" LBD is: " << LBD << std::endl;

        RsFile.close();
    }

    auto minLoadIter = std::min_element(bestHosts.begin(), bestHosts.end(),
                                        [](const auto& a, const auto& b) { return a.second < b.second; });

    auto maxLoadIter = std::max_element(bestHosts.begin(), bestHosts.end(),
                                        [](const auto& a, const auto& b) { return a.second < b.second; });

    /*double minLoad = minLoadIter->second;
    double maxLoad = maxLoadIter->second;

    // Compute the threshold
    double threshold = (maxLoad - minLoad) / 2;
*/
    RsFile.open("LBD.txt", std::ios::app);

    if (RsFile.is_open()) {

        RsFile << std::fixed << std::setprecision(10);  // Ensures 10 decimal places
        RsFile << "Threshold: " << LBDThreshold << std::endl;

        RsFile.close();
    }

    cModule* bestMecHost = nullptr;

    if (LBD > LBDThreshold) {
        bestMecHost = minLoadIter->first;  // Select MEC host with the smallest load
        //EV << "Selecting MEC Host with the smallest load: " << bestMecHost->getFullName() << "\n";

        Track.open("Track.txt", std::ios::app);

        if (Track.is_open()) {

            Track << std::fixed << std::setprecision(10);  // Ensures 10 decimal places
            Track << "Load Aware - Best Host is: " << bestMecHost << std::endl;

            Track.close();
        }

        return bestMecHost;
    }
    else{
        if (bestHosts.size() < 1)
           {
               return nullptr;
           }

        if (bestHosts.size() >= 2){
            // Sort the bestHosts by composite score in descending order and keep the top two
            std::sort(bestHosts.begin(), bestHosts.end(), [](const auto& a, const auto& b) {
                return a.second < b.second;
            });
            bestHosts.resize(2);
            RsFile.open("Load.txt", std::ios::app);

            if (RsFile.is_open()) {
                for (const auto& entry : bestHosts) {
                    RsFile << std::fixed << std::setprecision(10);  // Ensures 10 decimal places
                    RsFile << "Time:" << simTime() << " / " <<"The Best Hosts after Sorting: " << entry.first->getFullName() << " has load: " << entry.second << std::endl;

                }
                RsFile.close();
            }

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
                RsFile.open("Load.txt", std::ios::app);  // Open file for appending

                if (RsFile.is_open()) {
                       RsFile << std::fixed << std::setprecision(10);  // Ensures 10 decimal places
                       RsFile << "mecHost Name:" << mecHostName << "and mechost is : "<< mecHost <<std::endl;
                       RsFile.close();
                   }

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

                            RsFile.open("distance.txt", std::ios::app);

                            if (RsFile.is_open()) {
                                RsFile << "The mecHost : " << closestMecHost << " with distance: "<< distance << std::endl;
                                RsFile.close();
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
            RsFile.open("Score.txt", std::ios::app);
            if (RsFile.is_open()) {
                RsFile << "The minimum distance between vehicle and MEC host is : " << minDistance << " units" << std::endl;
                RsFile.close();
            }

            auto end = std::chrono::high_resolution_clock::now();
            long duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

            RsFile.open("Score.txt", std::ios::app);
            if (RsFile.is_open()) {
                RsFile << "The relocation duration is : " << duration << "ms" << std::endl;
                RsFile.close();
            }

            RsFile.open("Score.txt", std::ios::app);  // Open file for appending

            if (RsFile.is_open()) {
                RsFile << "The closest MEC Host is : " << closestMecHost << std::endl;
                RsFile.close();
            }
            ResourceDescriptor resources = appDesc.getVirtualResources();
            UTsFile.open("Util.txt", std::ios::app);  // Open file for appending
            if (UTsFile.is_open()) {
                UTsFile << "The : " << closestMecHost << " , "<< " CPU "  << resources.cpu << std::endl;
                UTsFile << "The : " << closestMecHost << " , "<< " RAM "  << resources.ram << std::endl;
                UTsFile << "The : " << closestMecHost << " , "<< " Disk "  << resources.disk << std::endl;
                UTsFile.close();
            }
            //VirtualisationInfrastructureManager* vim = check_and_cast<VirtualisationInfrastructureManager*>(closestMecHost->getSubmodule("vim"));
           // vim->PrintResources();
            Track.open("Track.txt", std::ios::app);  // Open file for appending

            if (Track.is_open()) {

                Track << std::fixed << std::setprecision(10);  // Ensures 10 decimal places
                Track << "Distance Aware - Best Host is: " << closestMecHost << std::endl;

                Track.close();
            }
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


   }
