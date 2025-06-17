#define _POSIX_C_SOURCE 200809L // Required for localtime_r on some systems for POSIX compliance
#include <iostream>
#include <fstream>
#include <string>
#include <ctime>

// Function to get the current Resident Set Size (RSS) memory usage in KB.
// RSS is the portion of memory held in RAM, not swapped.
long get_current_rss_kb() {
    std::ifstream ifs("/proc/self/status"); // Read from current process's status file
    std::string line;
    long rss_gb = 0;
    if (!ifs.is_open()) {
        std::cerr << "Error: Could not open /proc/self/status to get RSS." << std::endl;
        return 0;
    }
    while (std::getline(ifs, line)) {
        // Look for the "VmRSS:" line
        if (line.rfind("VmRSS:", 0) == 0) { // Check if line starts with "VmRSS:"
            size_t pos = line.find_first_of("0123456789"); // Find the first digit
            if (pos != std::string::npos) {
                try {
                    rss_gb = std::stol(line.substr(pos)); // Parse the number
                    //convert to GB
                    rss_gb /= (1024 * 1024); // Convert from KB to GB
                } catch (const std::exception& e) {
                    std::cerr << "Error parsing VmRSS: " << e.what() << std::endl;
                }
            }
            break; // Found VmRSS, no need to read further
        }
    }
    return rss_gb;
}

// Function to get the current number of threads spawned by the process.
int get_current_thread_count() {
    std::ifstream ifs("/proc/self/status"); // Read from current process's status file
    std::string line;
    int threads = 0;
    if (!ifs.is_open()) {
        std::cerr << "Error: Could not open /proc/self/status to get thread count." << std::endl;
        return 0;
    }
    while (std::getline(ifs, line)) {
        // Look for the "Threads:" line
        if (line.rfind("Threads:", 0) == 0) { // Check if line starts with "Threads:"
            size_t pos = line.find_first_of("0123456789"); // Find the first digit
            if (pos != std::string::npos) {
                try {
                    threads = std::stoi(line.substr(pos)); // Parse the number
                } catch (const std::exception& e) {
                    std::cerr << "Error parsing Threads: " << e.what() << std::endl;
                }
            }
            break; // Found Threads, no need to read further
        }
    }
    return threads;
}

