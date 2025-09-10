#include "renderstep.hpp"
#include <imgui.h>

NormalVisualizer::NormalVisualizer(std::string vertex_shader,
                                   std::string geometry_shader,
                                   std::string fragment_shader)
    : Renderstep(ProgramObject(
          {ShaderObject::FromFile(vertex_shader, GL_VERTEX_SHADER),
           ShaderObject::FromFile(geometry_shader, GL_GEOMETRY_SHADER),
           ShaderObject::FromFile(fragment_shader, GL_FRAGMENT_SHADER)})) {
    inputTextures.emplace_back("positions");
    inputTextures.emplace_back("normals");
    inputTextures.emplace_back("maks");

    glLineWidth(lineWidth);
    if (antiAliasing)
        glEnable(GL_LINE_SMOOTH);
    else
        glDisable(GL_LINE_SMOOTH);
}

NormalVisualizer::~NormalVisualizer() {}

std::string NormalVisualizer::name() const { return "Normal visualizer"; }

void NormalVisualizer::SetPositionsInputTexture(
    std::shared_ptr<VirtualTexture> tex) {
    inputTextures[0].Set(tex);
}

void NormalVisualizer::SetNormalsInputTexture(
    std::shared_ptr<VirtualTexture> tex) {
    inputTextures[1].Set(tex);
}

void NormalVisualizer::SetMasksInputTexture(
    std::shared_ptr<VirtualTexture> tex) {
    inputTextures[2].Set(tex);
}

void NormalVisualizer::Draw(CalibrationData &curCalib, Camera &curCam) {

    shader.Use();
    shader.SetTexture("positions", 0, *inputTextures[0].GetTex());
    shader.SetTexture("normals", 1, *inputTextures[1].GetTex());
    shader.SetTexture("mask", 2, *inputTextures[2].GetTex());
    shader.SetUniform("resolution", curCalib.resolution_px);
    shader.SetUniform("VP", curCam.GetVP());
    shader.SetUniform("length", length);
    shader.SetUniform("color", color);
    shader.SetUniform("nth", nth);
    shader.SetUniform("smart_invert", (int)smartInvert);
    shader.SetUniform("both_sides", (int)bothSides);
    shader.SetUniform("useMask", (int)useMask);
    shader.SetUniform("cam_pos", curCam.pos);
    shader.SetUniform("screen_space_length", (int)distance_independent_length);

    // default fbo already used
    // viewport size already set

    if (antiAliasing) {
        glEnable(GL_BLEND);
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO,
                            GL_ONE); // leave destination alpha untouched
    }

    glDrawArrays(GL_POINTS, 0,
                 curCalib.resolution_px.x * curCalib.resolution_px.y);

    if (antiAliasing) {
        glDisable(GL_BLEND);
    }
}

void NormalVisualizer::DrawGui() {
    ImGui::Checkbox("mask", &useMask);
    ImGui::InputInt2("display only nth", &nth[0]);
    ImGui::DragFloat("length", &length, 0.05f);
    ImGui::Checkbox("screen space length", &distance_independent_length);
    ImGui::ColorEdit3("color", &color[0],
                      ImGuiColorEditFlags_::ImGuiColorEditFlags_DisplayRGB);
    ImGui::Checkbox("Both sides", &bothSides);
    if (!bothSides)
        ImGui::Checkbox("Smart invert", &smartInvert);

    // if(ImGui::DragFloat("line width", &lineWidth, 0.01f))
    //     glLineWidth(lineWidth);
    if (ImGui::Checkbox("anti-aliasing", &antiAliasing)) {
        if (antiAliasing)
            glEnable(GL_LINE_SMOOTH);
        else
            glDisable(GL_LINE_SMOOTH);
    }
}