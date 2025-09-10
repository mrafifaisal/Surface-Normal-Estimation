#pragma once

#include <glm/glm.hpp>
#include <string>

struct CalibrationData {
    /// Distance between stereo cameras (in mm)
    float baseline_mm;

    /// Pixel coordinates of the principal point (screen center)
    /// Usually around the center, rectification changes it
    glm::vec2 center_px;

    /// Resolution of the images
    glm::ivec2 resolution_px;

    /// (1,1) element of camera intrinsic matrix (or horizontal focal length in
    /// pixels)
    float fx;
    /// (2,2) element of camera intrinisc matrix (or vertical focal length in
    /// pixels) Usually same as fx
    float fy;

    /// Horizontal difference of principal points (most of the time 0)
    float doffs = 0;

    /// Loads calibration data from file
    CalibrationData(const std::string &path);

    CalibrationData(float baseline_mm, glm::vec2 center_px,
                    glm::ivec2 resolution_px, float fx, float fy);

    /// Initialize from focal length (f), vertical field of view (fovy), 
    /// and aspect ratio (height/width)
    /// Assumes principal point is exacly in the center
    CalibrationData(float baseline_mm, glm::ivec2 resolution_px, float f,
                    float fovy, float aspect);

    /// Conversion method for display on UI
    std::string ToString() const;
};