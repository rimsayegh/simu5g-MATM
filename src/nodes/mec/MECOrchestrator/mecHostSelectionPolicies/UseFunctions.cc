#include <fstream>
#include <iomanip>
#include <string>
#include <map>
#include <random>
#include <iostream>
#include <sys/stat.h>
#include "common/binder/Binder.h"


using namespace omnetpp;
using namespace inet;
using namespace std;



#include <iostream>
#include <fstream>
#include <string>


#include <iostream>
#include <fstream>
#include <string>
#include <tuple>

// Function to read coordinates from the file and return x and y
std::tuple<int, int> readCoordinates(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Unable to open file!" << std::endl;
        return std::make_tuple(0, 0); // Return default values in case of error
    }

    int x = 0, y = 0;
    std::string line;
    while (std::getline(file, line)) {
        size_t start = line.find('[');  // Find the starting bracket
        size_t middle = line.find(';');  // Find the semicolon
        size_t end = line.find(']');     // Find the closing bracket

        if (start != std::string::npos && middle != std::string::npos && end != std::string::npos) {
            std::string x_str = line.substr(start + 1, middle - start - 1);  // Extract x
            std::string y_str = line.substr(middle + 1, end - middle - 1);   // Extract y

            x = std::stoi(x_str);  // Convert x to integer
            y = std::stoi(y_str);  // Convert y to integer
        }
    }

    file.close();
    return std::make_tuple(x, y);
}

