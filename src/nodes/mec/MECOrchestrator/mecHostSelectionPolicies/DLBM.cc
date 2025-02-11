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
#include "DLBM.h"


std::ofstream RsFile;
std::ofstream UTsFile;
std::ofstream Track;
std::ofstream csvFile;

std::vector<double> pastLBDs; // Store the past LBD values globally
std::vector<double> pastLoads;
std::vector<double> pastDistances;

DLBM::DLBM(MecOrchestrator* mecOrchestrator,double alpha, double betta, double lamda):SelectionPolicyBase(mecOrchestrator)
{
     alpha_ = alpha;
     betta_ = betta;
     lamda_ = lamda;
}

//Write in a csv file
void writeToCSV(simtime_t time, std::string Type, double Vehicle_x, double Vehicle_y, double Speed, double DataRate,double Distance1,double Distance2,double Distance3, double Min_Distance, double Max_Distance, double MEC_Load1,double MEC_Load2, double MEC_Load3, double Min_Load, double Max_Load, double LBD,
                double CF1, double CF2, cModule* Best_MEC, double alpha, double betta, double lamda) {
    csvFile.open("metrics_log.csv", std::ios::app);

    if (csvFile.is_open()) {
        csvFile << std::fixed << std::setprecision(10);

        static bool headerWritten = false;
        if (!headerWritten) {
            csvFile << "Time,Type,Vehicle_x,Vehicle_y,Speed,DataRate,Distance1,Distance2,Distance3,Min_Distance,Max_Distance,MEC_Load,Min_Load,Max_Load,LBD,CF1,CF2,Best_MEC,alpha,betta,lamda\n";
            headerWritten = true;
        }

        csvFile << time << ","
                << Type << ","
                << Vehicle_x << ","
                << Vehicle_y << ","
                << Speed << ","
                << DataRate << ","
                << Distance1 << ","
                << Distance2 << ","
                << Distance3 << ","
                << Min_Distance << ","
                << Max_Distance << ","
                << MEC_Load1 << ","
                << MEC_Load2 << ","
                << MEC_Load3 << ","
                << Min_Load << ","
                << Max_Load << ","
                << LBD << ","
                << CF1 << ","
                << CF2 << ","
                << Best_MEC << ","
                << alpha << ","
                << betta << ","
                << lamda << "\n";

        csvFile.close();
    } else {
        EV << "Error: Could not open CSV file!" << endl;
    }
}

//Read the locations of BSs
std::vector<std::pair<std::string, std::pair<double, double>>> readBaseStations(const std::string &filename) {
    std::ifstream bsFile(filename);
    std::vector<std::pair<std::string, std::pair<double, double>>> baseStations;

    if (!bsFile.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return baseStations;  // Return empty vector if file cannot be opened
    }

    std::string line;
    while (std::getline(bsFile, line)) {
        size_t pos = line.find(":");
        if (pos == std::string::npos) continue; // Skip invalid lines

        std::string bsName = line.substr(0, pos);
        size_t coordStart = line.find("[");

        if (coordStart != std::string::npos) {
            size_t coordEnd = line.find("]");
            if (coordEnd == std::string::npos) continue; // Skip invalid lines

            std::string coords = line.substr(coordStart + 1, coordEnd - coordStart - 1);
            size_t sep = coords.find(";");
            if (sep == std::string::npos) continue; // Skip invalid lines

            try {
                double x = std::stod(coords.substr(0, sep));
                double y = std::stod(coords.substr(sep + 1));
                baseStations.emplace_back(bsName, std::make_pair(x, y));
            } catch (const std::exception &e) {
                std::cerr << "Error parsing coordinates for " << bsName << ": " << e.what() << std::endl;
                continue;
            }
        }
    }

    bsFile.close();
    return baseStations;
}


// Function to remove the first line from a given file
void removeFirstLineFromFile(const std::string& filename) {
    std::ifstream inFile(filename);
    std::ofstream tempFile("Vehicle_temp.txt");

    if (!inFile.is_open() || !tempFile.is_open()) {
        std::cerr << "Error: Unable to open file " << filename << std::endl;
        return;
    }

    std::string line;
    bool isFirstLine = true;

    while (std::getline(inFile, line)) {
        if (isFirstLine) {
            isFirstLine = false;
            continue;  // Skip the first line
        }
        tempFile << line << std::endl;
    }

    inFile.close();
    tempFile.close();

    // Replace original file with the new file
    if (std::remove(filename.c_str()) != 0) {
        std::cerr << "Error: Unable to delete original file " << filename << std::endl;
        return;
    }

    if (std::rename("Vehicle_temp.txt", filename.c_str()) != 0) {
        std::cerr << "Error: Unable to rename temporary file" << std::endl;
    }
}


double calculateLoadBalancingDegree(const std::vector<std::tuple<cModule*, double, double, double, double>>& bestHosts) {
    int K = bestHosts.size();
    if (K == 0) return 0.0;  // Avoid division by zero

    // Step 1: Compute the mean Load
    double sumLoad = 0.0;
    for (const auto& entry : bestHosts) {
        sumLoad += std::get<1>(entry);  // Load is the 2nd element in the tuple
    }
    double meanLoad = sumLoad / K;

    // Step 2: Compute the variance (LBD formula)
    double variance = 0.0;
    for (const auto& entry : bestHosts) {
        variance += pow(std::get<1>(entry) - meanLoad, 2);  // Load is the 2nd element
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

cModule* DLBM::findBestMecHost(const ApplicationDescriptor& appDesc)
{
    auto start = std::chrono::high_resolution_clock::now();
    cModule* bestHost = nullptr;
    double maxCompositeScore = -1;
    std::vector<std::tuple<cModule*, double, double>> MEC;

    std::vector<std::tuple<cModule*, double, double, double, double>> bestHosts;

    // Read Base Stations from BS.txt
    auto baseStations = readBaseStations("BS.txt");


    // Read Vehicle coordinates from Vehicle.txt
    std::string line;
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

    //Reat the Speed and data rate
    std::ifstream vFile("Vinfo.txt");
        double Speed = 0;
        double DataRate = 0;
        if (std::getline(vFile, line))
        {
            std::stringstream ss(line);
            std::string temp;

            while (ss >> temp) {  // Read words
                if (temp == "Speed:") ss >> Speed;
                if (temp == "DataRate:") ss >> DataRate;
            }
        }
        vFile.close();
        removeFirstLineFromFile("Vinfo.txt");

    int Flag = 0;

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
                //Calculate the Load and the Distance
                double load = vim->CalculateLoad();

                //Distance
                double distance = 0;

                std::string mecHostName = mecHost->getName();

                for (const auto& baseStation : baseStations)
                {
                    if (mecHostName == "mecHost" + baseStation.first.substr(2))
                    {
                        double bsX = baseStation.second.first;
                        double bsY = baseStation.second.second;
                        distance = calculateDistance(vehicleX, vehicleY, bsX, bsY);
                        break;
                    }
                }
                /////Distance

                pastLoads.push_back(load);
                pastDistances.push_back(distance);


                MEC.push_back(std::make_tuple(mecHost, load, distance));

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

    //Calulate the cost functions
    double minLoad = 0;
    double maxLoad = 0;
    double minDistance = 0;
    double maxDistance = 0;
    double load_norm = 0;
    double distance_norm = 0;

    if (!pastLoads.empty()) {
               // Compute min/max dynamically before each allocation
               minLoad = *std::min_element(pastLoads.begin(), pastLoads.end());
               maxLoad = *std::max_element(pastLoads.begin(), pastLoads.end());

               minDistance = *std::min_element(pastDistances.begin(), pastDistances.end());
               maxDistance = *std::max_element(pastDistances.begin(), pastDistances.end());

               Flag =1;
           }

    for (const auto& entry : MEC) {
            cModule* mecHost = std::get<0>(entry);
            double load = std::get<1>(entry);
            double distance = std::get<2>(entry);

            if (pastLoads.empty()) {
                //double minLoad = maxLoad = load;
                //double minDistance = maxDistance = distance;
                load_norm = 0;
                distance_norm = 0;

            }

            if(maxLoad == minLoad){
                maxLoad += 0.001;
            }
            if(maxDistance == minDistance){
                maxDistance += 1;
            }

            // Compute normalized values
            double load_norm = (load - minLoad) / (maxLoad - minLoad);
            double distance_norm = (distance - minDistance) / (maxDistance - minDistance);

            // Compute Cost Functions
            double CF1 = alpha_ * load_norm + (1 - alpha_) * distance_norm;
            double CF2 = betta_ * distance_norm + (1 - betta_) * load_norm;

            // Store results in bestHosts
            bestHosts.push_back(std::make_tuple(mecHost, load, distance, CF1, CF2));

            RsFile.open("vector.txt", std::ios::app);

            if (RsFile.is_open()) {
               RsFile << std::fixed << std::setprecision(10);  // Ensures 10 decimal places
               RsFile << "Time:" << simTime() << " / " <<"The mecHost: " << mecHost->getFullPath() << " has load: " << load << ", distance: " << distance << ", CF1: "<< CF1 << ", CF2: " << CF2 << std::endl;
               RsFile.close();
            }
        }

    //Calculate the Load Balance degree
    double LBD = calculateLoadBalancingDegree(bestHosts);

    // Store the new LBD in the history
    pastLBDs.push_back(LBD);


    double meanLBD = calculateMean(pastLBDs);
    double stdDevLBD = calculateStdDev(pastLBDs, meanLBD);

    //sensitivity factor
    //double lambda = 2;  //adjust this based on testing

    // Compute the dynamic threshold for LBD
    double LBDThreshold = meanLBD + lamda_ * stdDevLBD;


    RsFile.open("LBD.txt", std::ios::app);

    if (RsFile.is_open()) {

        RsFile << std::fixed << std::setprecision(10);  // Ensures 10 decimal places
        RsFile << "Time:" << simTime() << " / " <<" LBD is: " << LBD << std::endl;

        RsFile.close();
    }


    RsFile.open("LBD.txt", std::ios::app);

    if (RsFile.is_open()) {

        RsFile << std::fixed << std::setprecision(10);  // Ensures 10 decimal places
        RsFile << "Threshold: " << LBDThreshold << std::endl;

        RsFile.close();
    }

    cModule* bestMecHost = nullptr;


    if (LBD > LBDThreshold) {
        //bestMecHost = minLoadIter->first;  // Select MEC host with the smallest load

        //Sort the MEChsots based on the Load-Based Cost Function CF1
        std::sort(bestHosts.begin(), bestHosts.end(), [](const auto& a, const auto& b) {
            return std::get<3>(a) < std::get<3>(b);  // Compare CF1 (4th element)
        });

        // Retrieve the MEC host with the lowest CF1
        cModule* bestMecHost = std::get<0>(bestHosts.front());

        removeFirstLineFromFile("Vehicle.txt");

        Track.open("Track.txt", std::ios::app);

        if (Track.is_open()) {

            Track << std::fixed << std::setprecision(10);  // Ensures 10 decimal places
            Track << "Load Aware - Best Host is: " << bestMecHost << std::endl;

            Track.close();
        }
        //Save the data in the dataset
        /*std::string Type;
        Type = "Load Aware";
        double bestDistance = std::get<2>(bestHosts[0]);
        double Distance2 = std::get<2>(bestHosts[1]);
        double Distance3 = std::get<2>(bestHosts[2]);
        double bestLoad = std::get<1>(bestHosts[0]);
        double Load2 = std::get<1>(bestHosts[1]);
        double Load3 = std::get<1>(bestHosts[2]);
        double C1 = std::get<3>(bestHosts[0]);
        double C2 = std::get<4>(bestHosts[0]);
        writeToCSV(simTime(), Type, vehicleX, vehicleY, Speed, DataRate, bestDistance,Distance2, Distance3, minDistance, maxDistance, bestLoad,Load2, Load3, minLoad, maxLoad, LBD, C1, C2, bestMecHost, alpha_, betta_, lamda_);
*/
        return bestMecHost;
    }
    else{
        cModule* closestMecHost = nullptr;

        //Sort the MEChsots based on the Distance-Based Cost Function CF2
        std::sort(bestHosts.begin(), bestHosts.end(), [](const auto& a, const auto& b) {
            return std::get<4>(a) < std::get<4>(b);  // Compare CF2 (5th element)
        });

        // Retrieve the MEC host with the lowest CF2
        closestMecHost = std::get<0>(bestHosts.front());




        // Delete first row from Vehicle.txt
        removeFirstLineFromFile("Vehicle.txt");


        // Log the minimum distance
        RsFile.open("Score.txt", std::ios::app);
        if (RsFile.is_open()) {
            RsFile << "The minimum distance between vehicle and MEC host is : " << std::get<2>(bestHosts.front()) << " units" << std::endl;
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
        //Add Data to the Dataset
        /*std::string Type;
        Type = "Distance Aware";
        double bestDistance = std::get<2>(bestHosts[0]);
        double Distance2 = std::get<2>(bestHosts[1]);
        double Distance3 = std::get<2>(bestHosts[2]);
        double bestLoad = std::get<1>(bestHosts[0]);
        double Load2 = std::get<1>(bestHosts[1]);
        double Load3 = std::get<1>(bestHosts[2]);
        double C1 = std::get<3>(bestHosts[0]);
        double C2 = std::get<4>(bestHosts[0]);
        writeToCSV(simTime(), Type, vehicleX, vehicleY, Speed, DataRate, bestDistance,Distance2, Distance3, minDistance, maxDistance, bestLoad,Load2, Load3, minLoad, maxLoad, LBD, C1, C2, closestMecHost, alpha_, betta_, lamda_);
*/
        return closestMecHost;  // Return the closest MEC host
    }

    cModule* mecHost = std::get<0>(bestHosts.front());

    return mecHost;


    }



