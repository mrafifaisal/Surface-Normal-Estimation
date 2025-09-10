#include "renderstep.hpp"
#include <imgui.h>

CrossNormalEstimator::CrossNormalEstimator(glm::ivec2 outputres,
                                           std::string screen_vertex_shader,
                                           std::string cne_fragment_shader)
    : Renderstep(
          {ShaderObject::FromFile(screen_vertex_shader, GL_VERTEX_SHADER),
           ShaderObject::FromFile(cne_fragment_shader, GL_FRAGMENT_SHADER)}) {
    inputTextures.emplace_back("positions");
    normTex = std::make_shared<VirtualTexture>("cne normal");
    outputTextures.push_back(normTex);

    ResizeOwnedTextures(outputres);
}

CrossNormalEstimator::~CrossNormalEstimator() {}

std::string CrossNormalEstimator::name() const {
    return "Cross normal estimator";
}

void CrossNormalEstimator::SetPositionsInputTexture(
    std::shared_ptr<VirtualTexture> tex) {
    inputTextures[0].Set(tex);
}

void CrossNormalEstimator::ResizeOwnedTextures(glm::ivec2 newres) {
    normTex->assign(std::make_shared<Texture2D>(newres.x, newres.y, GL_RGBA32F));
    fbo = std::unique_ptr<FrameBufferObject>(new FrameBufferObject(
        {{GL_COLOR_ATTACHMENT0, *normTex->getCurrent()}}));
}

void CrossNormalEstimator::Draw(CalibrationData &curCalib, Camera &curCam) {
    shader.Use();
    shader.SetTexture("positions", 0, *inputTextures[0].GetTex());
    shader.SetUniform("resolution", curCalib.resolution_px);
    shader.SetUniform("symmetric", (int)symmetric_mode);

    fbo->Bind();
    // viewport size already set?

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void CrossNormalEstimator::DrawGui() {
    ImGui::Checkbox("use symmetric difference", &symmetric_mode);
}