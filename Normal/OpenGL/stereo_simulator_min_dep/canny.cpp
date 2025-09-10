#include "canny.hpp"
#include <imgui.h>

CannyEdgeDetector::CannyEdgeDetector(glm::ivec2 outputres,
                                     std::string screen_vertex_shader,
                                     std::string canny_prepare_fragment_shader,
                                     std::string canny_fragment_shader)
    : Renderstep(
          {ShaderObject::FromFile(screen_vertex_shader, GL_VERTEX_SHADER),
           ShaderObject::FromFile(canny_fragment_shader, GL_FRAGMENT_SHADER)},
          true),
      screen_shader_location(screen_vertex_shader),
      prepare_shader_location(canny_prepare_fragment_shader),
      canny_shader_location(canny_fragment_shader),
      prepareShader(
          {ShaderObject::FromFile(screen_vertex_shader, GL_VERTEX_SHADER),
           ShaderObject::FromFile(canny_prepare_fragment_shader,
                                  GL_FRAGMENT_SHADER)}) {
    inputTextures.emplace_back("input");

    preparedTex = std::make_shared<VirtualTexture>("Prepared depth");

    edgeTex = std::make_shared<VirtualTexture>("Canny edges");
    outputTextures.push_back(edgeTex);

    ResizeOwnedTextures(outputres);
}

CannyEdgeDetector::~CannyEdgeDetector() {}

std::string CannyEdgeDetector::name() const { return "Canny"; }

void CannyEdgeDetector::ResizeOwnedTextures(glm::ivec2 newres) {
    preparedTex->assign(
        std::make_shared<Texture2D>(newres.x, newres.y, GL_RGBA32F));
    prepareFbo = std::unique_ptr<FrameBufferObject>(new FrameBufferObject(
        {{GL_COLOR_ATTACHMENT0, *preparedTex->getCurrent()}}));
    edgeTex->assign(std::make_shared<Texture2D>(newres.x, newres.y, GL_RGBA32F));
    edgeFbo = std::unique_ptr<FrameBufferObject>(new FrameBufferObject(
        {{GL_COLOR_ATTACHMENT0, *edgeTex->getCurrent()}}));
}

void CannyEdgeDetector::Draw(CalibrationData &curCalib, Camera &curCam) {
    prepareShader.Use();
    shader.SetTexture("tex", 0, *inputTextures[0].GetTex());
    prepareFbo->Bind();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    shader.Use();
    shader.SetTexture("tex", 0, *preparedTex->getCurrent());
    shader.SetUniform("res", curCalib.resolution_px);
    shader.SetUniform("weakThreshold", weakThreshold);
    shader.SetUniform("strongThreshold", strongThreshold);

    edgeFbo->Bind();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void CannyEdgeDetector::DrawGui() {
    ImGui::DragFloat("weak thresh", &weakThreshold);
    ImGui::DragFloat("strong thresh", &strongThreshold);
}

void CannyEdgeDetector::SetTexture(std::shared_ptr<VirtualTexture> tex) {
    inputTextures[0].Set(tex);
}

void CannyEdgeDetector::ReloadShaders() {
    prepareShader = {
        ShaderObject::FromFile(screen_shader_location, GL_VERTEX_SHADER),
        ShaderObject::FromFile(prepare_shader_location, GL_FRAGMENT_SHADER)};
    shader = {
        ShaderObject::FromFile(screen_shader_location, GL_VERTEX_SHADER),
        ShaderObject::FromFile(canny_shader_location, GL_FRAGMENT_SHADER)};
}