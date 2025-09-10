#include "renderstep.hpp"
#include <imgui.h>

AffineNormalEstimator::AffineNormalEstimator(glm::ivec2 outputres,
                                             std::string screen_vertex_shader,
                                             std::string cne_fragment_shader)
    : Renderstep(
          {ShaderObject::FromFile(screen_vertex_shader, GL_VERTEX_SHADER),
           ShaderObject::FromFile(cne_fragment_shader, GL_FRAGMENT_SHADER)}) {
    inputTextures.emplace_back("affine parameters");
    inputTextures.emplace_back("positions");
    normTex = std::make_shared<VirtualTexture>("affine normal");
    outputTextures.push_back(normTex);

    ResizeOwnedTextures(outputres);
}

AffineNormalEstimator::~AffineNormalEstimator() {}

std::string AffineNormalEstimator::name() const {
    return "Affine normal estimator";
}

void AffineNormalEstimator::SetAffineParameterInputTexture(
    std::shared_ptr<VirtualTexture> tex) {
    inputTextures[0].Set(tex);
}

void AffineNormalEstimator::SetPositionsInputTexture(
    std::shared_ptr<VirtualTexture> tex) {
    inputTextures[1].Set(tex);
}

void AffineNormalEstimator::ResizeOwnedTextures(glm::ivec2 newres) {
    normTex->assign(std::make_shared<Texture2D>(newres.x, newres.y, GL_RGBA32F));
    fbo = std::unique_ptr<FrameBufferObject>(new FrameBufferObject(
        {{GL_COLOR_ATTACHMENT0, *normTex->getCurrent()}}));
}

void AffineNormalEstimator::Draw(CalibrationData &curCalib, Camera &curCam) {
    shader.Use();
    shader.SetTexture("affine_params", 0, *inputTextures[0].GetTex());
    shader.SetTexture("positions", 1, *inputTextures[1].GetTex());
    shader.SetUniform("fx", curCalib.fx);
    shader.SetUniform("fy", curCalib.fy);
    shader.SetUniform("baseline", curCalib.baseline_mm / 1000.0f);

    fbo->Bind();
    // viewport size already set

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void AffineNormalEstimator::DrawGui() {
    
}