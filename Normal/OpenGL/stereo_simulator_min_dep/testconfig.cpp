#include "testconfig.hpp"
#include <iostream>
#include <map>

#define NAME_OF(x) #x
#define ADD_MEMBER(x)                                                          \
    {                                                                          \
        #x, { offsetof(TestConfig, x), typeid(decltype(x)).hash_code() }       \
    }
#define PRINT_MEMBER(x,s) s << NAME_OF(x) << '=' << x << '\n'

void TestConfig::ReadParams(std::string file) {
    std::ifstream c(file);
    while (!c.eof()) {
        std::string line;
        std::getline(c, line);
        if (line.size() == 0 || line[0] == '#')
            continue;

        int idx = line.find('=');
        if (idx != std::string::npos) {
            std::string key = line.substr(0, idx);
            std::string value = line.substr(idx + 1);
            AssignValue(key,value);
        }
    }
}

void TestConfig::AssignValue(const std::string &key, const std::string &value) {

    //                varname             <offset, type hash>
    static std::map<std::string, std::pair<size_t, size_t>> valmap = {
        // {NAME_OF(method), {offsetof(TestConfig, method),
        // typeid(decltype(method)).name()}},
        ADD_MEMBER(method),
        ADD_MEMBER(affine_mode),
        ADD_MEMBER(adaptive_dir_count),
        ADD_MEMBER(adaptive_max_step_count),
        ADD_MEMBER(adaptive_depth_thresh_ratio),
        ADD_MEMBER(conv_size),
        ADD_MEMBER(adaptive_simple_thresh),
        ADD_MEMBER(adaptive_cumulative_thresh),
        ADD_MEMBER(test_deg_thresh),
        ADD_MEMBER(pca_size),
        ADD_MEMBER(pca_version)};

    auto record_it = valmap.find(key);
    if (record_it == valmap.end()) {
        std::cerr << "Error parsing test configuration, unknown key: " << key
                  << ", ignoring line\n";
    }

    auto description = (*record_it).second;
    void *target_addr = (void*)this + description.first;
    
    if (description.second == typeid(int).hash_code())
        *(int*)target_addr = std::stoi(value);
    else if(description.second == typeid(float).hash_code()){
        float parsed = std::stof(value);
        *(float*)target_addr = parsed;
    }
    else if(description.second == typeid(bool).hash_code())
        *(bool*)target_addr = (value == "true" ? true : false);
}

void TestConfig::PrintConfig() const {
    PRINT_MEMBER(method,std::cout);
    PRINT_MEMBER(affine_mode,std::cout);
    PRINT_MEMBER(adaptive_dir_count,std::cout);
    PRINT_MEMBER(adaptive_max_step_count,std::cout);
    PRINT_MEMBER(adaptive_depth_thresh_ratio,std::cout);
    PRINT_MEMBER(conv_size,std::cout);
    PRINT_MEMBER(adaptive_simple_thresh,std::cout);
    PRINT_MEMBER(adaptive_cumulative_thresh,std::cout);
    PRINT_MEMBER(test_deg_thresh,std::cout);
    PRINT_MEMBER(pca_size,std::cout);
    PRINT_MEMBER(pca_version,std::cout);
}