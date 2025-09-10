#include "renderstep.hpp"
#include <imgui.h>
#include <iostream>

struct MeasureWGResult {
    unsigned int count;
    double sum;
};

MeasureDifference::MeasureDifference(glm::ivec2 resolution,
                                     std::string compute_shader)
    : Renderstep({ShaderObject::FromFile(compute_shader, GL_COMPUTE_SHADER)}) {
    inputTextures.emplace_back("reference");
    inputTextures.emplace_back("observed");
    // inputTextures.emplace_back("mask");

    ResizeOwnedTextures(resolution);
}

MeasureDifference::~MeasureDifference() { glDeleteBuffers(1, &bufferID); }

std::string MeasureDifference::name() const { return "Measure error"; }

void MeasureDifference::SetComparisonTexture(
    std::shared_ptr<VirtualTexture> tex, bool reference) {
    inputTextures[reference ? 0 : 1].Set(tex);
}

void MeasureDifference::SetMaskTexture(std::shared_ptr<VirtualTexture> tex) {
    inputTextures[2].Set(tex);
}

void MeasureDifference::ResizeOwnedTextures(glm::ivec2 newres) {
    if(bufferID != GL_NONE)
        glDeleteBuffers(1, &bufferID);

    int wsx = newres.x / 32, wsy = newres.y / 32;
    if (newres.x % 32 != 0)
        wsx += 1;
    if (newres.y % 32 != 0)
        wsy += 1;

    glGenBuffers(1, &bufferID);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, bufferID);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(MeasureWGResult) * wsx * wsy, nullptr,
                 GL_DYNAMIC_READ);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void MeasureDifference::Draw(CalibrationData &curCalib, Camera &curCam) {
    if (!inputTextures[0].IsSet() || !inputTextures[1].IsSet())
        return;

    shader.Use();

    shader.SetUniform("resolution", curCalib.resolution_px);
    shader.SetUniform("metric", (int)metric);

    glBindImageTexture(0, *inputTextures[0].GetTex(), 0, GL_FALSE, 0,
                       GL_READ_ONLY, GL_RGBA32F);
    glBindImageTexture(1, *inputTextures[1].GetTex(), 0, GL_FALSE, 0,
                       GL_READ_ONLY, GL_RGBA32F);
    // glBindImageTexture(2, *inputTextures[2].GetTex(), 0, GL_FALSE, 0,
    //                    GL_READ_ONLY, GL_RGBA32F);
    shader.SetUniform("reference", 0);
    shader.SetUniform("observed", 1);
    // shader.SetUniform("mask", 2);
    shader.SetUniform("threshold", useThreshold ? threshold : -1.0);
    shader.SetUniform("margin", margin);
    // shader.SetUniform("use_mask", useMask ? 1 : 0);

    // glBindBuffer(GL_SHADER_STORAGE_BUFFER, bufferID);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, bufferID);

    int wsx = curCalib.resolution_px.x / 32,
        wsy = curCalib.resolution_px.y / 32;
    if (curCalib.resolution_px.x % 32 != 0)
        wsx += 1;
    if (curCalib.resolution_px.y % 32 != 0)
        wsy += 1;
    glDispatchCompute(wsx, wsy, 1);

    glMemoryBarrier(GL_ALL_BARRIER_BITS);

    std::vector<MeasureWGResult> v(wsx * wsy);
    // glGetBufferSubData(bufferID, 0, sizeof(double), v.data());
    // // glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    // glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, bufferID);
    void *buff_ptr = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    MeasureWGResult *d = (MeasureWGResult *)buff_ptr;
    memcpy(v.data(), buff_ptr, sizeof(MeasureWGResult) * wsx * wsy);
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    double result = 0;
    unsigned long long int count = 0;
    for (auto a : v) {
        result += a.sum;
        count += a.count;
        // std::cout << a.count << ":" << a.sum << " ";
    }
    std::cout << "\n";

    lastMeasurement =
        result / ((double)count);
    // lastMeasurement = result;
}

void MeasureDifference::DrawGui() {
    ImGui::Combo("Mode", (int *)&metric, option_names, 2);
    ImGui::PushID(1);
    ImGui::Checkbox("threshold", &useThreshold);
    // ImGui::Checkbox("use mask", &useMask);
    if(useThreshold){
        ImGui::SameLine();
        ImGui::InputDouble("", &threshold);
    }
    ImGui::PopID();
    ImGui::InputInt("margin", &margin);
    ImGui::Text("avg: %f", lastMeasurement);
}