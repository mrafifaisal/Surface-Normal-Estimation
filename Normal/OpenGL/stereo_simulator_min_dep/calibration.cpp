#include <fstream>
#include <sstream>

#include "calibration.hpp"

CalibrationData::CalibrationData(const std::string &path) {
    std::ifstream file(path);
    file >> baseline_mm;
    file >> center_px.x >> center_px.y;
    file >> resolution_px.x >> resolution_px.y;
    file >> fx;
    file >> fy;
    file >> doffs;
    file.close();
}

CalibrationData::CalibrationData(float baseline_mm, glm::vec2 center_px,
                                 glm::ivec2 resolution_px, float fx, float fy)
    : baseline_mm(baseline_mm), center_px(center_px),
      resolution_px(resolution_px), fx(fx), fy(fy) {}

CalibrationData::CalibrationData(float baseline_mm, glm::ivec2 resolution_px,
                                 float f, float fovy, float aspect)
    : baseline_mm(baseline_mm), resolution_px(resolution_px) {
    center_px = glm::vec2(resolution_px.x / 2.0f, resolution_px.y / 2.0f);

    float h = tan(fovy / 2) * f * 2;
    float w = aspect * h;

    fx = resolution_px.x / w * f;
    fy = resolution_px.y / h * f;
}

std::string CalibrationData::ToString() const {
    std::stringstream s;
    s << "fx: " << fx << " px\n";
    s << "fy: " << fy << " px\n";
    s << "b: " << baseline_mm << " mm\n";
    s << "res: " << resolution_px.x << "X" << resolution_px.y << "\n";
    s << "c: " << center_px.x << ", " << center_px.y << "\n";
    return s.str();
}