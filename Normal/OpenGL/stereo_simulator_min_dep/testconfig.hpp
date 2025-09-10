#pragma once
#include <fstream>
#include <string>
#include <utility>

// DO NOT FORGET TO EXTEND 'PrintConfig' AND 'AssignValue' WHEN ADDING A MEMBER
struct TestConfig {

    // Passed as cmdline arguments
    std::string disparity_file;
    std::string calibration_file;
    std::string gtnormal_fila;
    std::string mask_file;

    // read from file passed as argument
    int method = 0;      // 0 affine, 1 cross, 2 pca
    int affine_mode = 3; // 0 normal, 1 lsq, 2 adaptive cumulative lsq, 3
                         // adaptive CD, 4 adaptive angle, 5 adaptive ST
    int adaptive_dir_count = 8;
    int adaptive_max_step_count = 5;
    float adaptive_depth_thresh_ratio = 0.1f;
    int conv_size = 3;

    float adaptive_simple_thresh = 0.5f;
    float adaptive_cumulative_thresh = 1.0f;
    float test_deg_thresh = 10;

    int pca_size = 5;
    int pca_version = 0; // 0 modified svd, 1 Viktor's

    void ReadParams(std::string file);
    void PrintConfig() const;

  private:
    void AssignValue(const std::string &key, const std::string &value);
};