#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <limits>
#include <thread>
#include <vector>
#include <mutex>
#include <filesystem>
#include <future>

struct ShardResult {
    std::string filename;
    bool is_sorted;
    std::string error_message;
};

bool is_graph_sorted(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return false;
    }
    
    std::string line;
    int prev_src = std::numeric_limits<int>::min();
    int prev_dst = std::numeric_limits<int>::min();
    bool first_edge = true;
    
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        std::istringstream iss(line);
        int src, dst;
        
        if (!(iss >> src >> dst)) {
            std::cerr << "Error: Invalid line format: " << line << std::endl;
            return false;
        }
        
        if (first_edge) {
            prev_src = src;
            prev_dst = dst;
            first_edge = false;
            continue;
        }
        
        if (src < prev_src) {
            std::cout << "Graph is NOT sorted: source " << src 
                      << " comes after source " << prev_src << std::endl;
            return false;
        }
        
        if (src == prev_src && dst < prev_dst) {
            std::cout << "Graph is NOT sorted: for source " << src 
                      << ", destination " << dst << " comes after destination " 
                      << prev_dst << std::endl;
            return false;
        }
        
        prev_src = src;
        prev_dst = dst;
    }
    
    file.close();
    return true;
}

ShardResult check_shard(const std::string& shard_path) {
    ShardResult result;
    result.filename = shard_path;
    result.is_sorted = true;
    result.error_message = "";
    
    std::ifstream file(shard_path);
    if (!file.is_open()) {
        result.is_sorted = false;
        result.error_message = "Could not open file";
        return result;
    }
    
    std::string line;
    int prev_src = std::numeric_limits<int>::min();
    int prev_dst = std::numeric_limits<int>::min();
    bool first_edge = true;
    
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        std::istringstream iss(line);
        int src, dst;
        
        if (!(iss >> src >> dst)) {
            result.is_sorted = false;
            result.error_message = "Invalid line format: " + line;
            return result;
        }
        
        if (first_edge) {
            prev_src = src;
            prev_dst = dst;
            first_edge = false;
            continue;
        }
        
        if (src < prev_src) {
            result.is_sorted = false;
            result.error_message = "Source " + std::to_string(src) + 
                                 " comes after source " + std::to_string(prev_src);
            return result;
        }
        
        if (src == prev_src && dst < prev_dst) {
            result.is_sorted = false;
            result.error_message = "For source " + std::to_string(src) + 
                                 ", destination " + std::to_string(dst) + 
                                 " comes after destination " + std::to_string(prev_dst);
            return result;
        }
        
        prev_src = src;
        prev_dst = dst;
    }
    
    return result;
}

std::vector<std::string> find_shard_files(const std::string& original_path) {
    std::filesystem::path path(original_path);
    std::string base_name = path.stem().string();
    std::string parent_dir = path.parent_path().string();
    std::string preprocess_dir = parent_dir + "/preprocess";
    
    std::vector<std::string> shard_files;
    
    if (!std::filesystem::exists(preprocess_dir)) {
        std::cerr << "Preprocess directory does not exist: " << preprocess_dir << std::endl;
        return shard_files;
    }
    
    for (char suffix = 'a'; suffix <= 'p'; suffix++) {
        std::string shard_name = base_name + "_a" + suffix;
        std::string shard_path = preprocess_dir + "/" + shard_name;
        
        if (std::filesystem::exists(shard_path)) {
            shard_files.push_back(shard_path);
        }
    }
    
    return shard_files;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <graph_file>" << std::endl;
        return 1;
    }
    
    std::string filename = argv[1];
    
    std::vector<std::string> shard_files = find_shard_files(filename);
    
    if (shard_files.empty()) {
        std::cout << "No shard files found, checking original file..." << std::endl;
        if (is_graph_sorted(filename)) {
            std::cout << "Graph is sorted!" << std::endl;
            return 0;
        } else {
            std::cout << "Graph is NOT sorted!" << std::endl;
            return 1;
        }
    }
    
    std::cout << "Found " << shard_files.size() << " shard files. Checking in parallel..." << std::endl;
    
    std::vector<std::future<ShardResult>> futures;
    
    for (const auto& shard_file : shard_files) {
        futures.push_back(std::async(std::launch::async, check_shard, shard_file));
    }
    
    bool all_sorted = true;
    std::vector<ShardResult> results;
    
    for (auto& future : futures) {
        ShardResult result = future.get();
        results.push_back(result);
        if (!result.is_sorted) {
            all_sorted = false;
        }
    }
    
    for (const auto& result : results) {
        if (result.is_sorted) {
            std::cout << "✓ " << std::filesystem::path(result.filename).filename().string() 
                      << " is sorted" << std::endl;
        } else {
            std::cout << "✗ " << std::filesystem::path(result.filename).filename().string() 
                      << " is NOT sorted: " << result.error_message << std::endl;
        }
    }
    
    if (all_sorted) {
        std::cout << "\nAll shards are sorted!" << std::endl;
        return 0;
    } else {
        std::cout << "\nSome shards are NOT sorted!" << std::endl;
        return 1;
    }
}