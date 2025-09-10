#include "pca.hpp"
#include <imgui.h>

PrincipalComponentAnalysisNE::PrincipalComponentAnalysisNE(
    glm::ivec2 outputres, std::string screen_vertex_shader,
    std::string pca_fragment_shader)
    : Renderstep(
          {ShaderObject::FromFile(screen_vertex_shader, GL_VERTEX_SHADER),
           ShaderObject::FromFile(pca_fragment_shader, GL_FRAGMENT_SHADER)},
          true),
      screen_shader_location(screen_vertex_shader),
      pca_shader_location(pca_fragment_shader) {
    inputTextures.emplace_back("positions");

    normalTex = std::make_shared<VirtualTexture>("PCA normal");
    outputTextures.push_back(normalTex);

    ResizeOwnedTextures(outputres);
}

PrincipalComponentAnalysisNE::~PrincipalComponentAnalysisNE() {}

std::string PrincipalComponentAnalysisNE::name() const { return "PCA"; }

void PrincipalComponentAnalysisNE::ResizeOwnedTextures(glm::ivec2 newres) {
    normalTex->assign(
        std::make_shared<Texture2D>(newres.x, newres.y, GL_RGBA32F));
    normalFbo = std::unique_ptr<FrameBufferObject>(new FrameBufferObject(
        {{GL_COLOR_ATTACHMENT0, *normalTex->getCurrent()}}));
}

void PrincipalComponentAnalysisNE::Draw(CalibrationData &curCalib,
                                        Camera &curCam) {
    shader.Use();
    shader.SetTexture("positions", 0, *inputTextures[0].GetTex());
    shader.SetUniform("resolution", curCalib.resolution_px);
    shader.SetUniform("square_size", size);
    shader.SetUniform("version", version);

    normalFbo->Bind();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void PrincipalComponentAnalysisNE::DrawGui() {
    ImGui::InputInt("size", &size);
    ImGui::Combo("version", &version, version_names, 2);
}

void PrincipalComponentAnalysisNE::SetPositionTexture(
    std::shared_ptr<VirtualTexture> tex) {
    inputTextures[0].Set(tex);
}

void PrincipalComponentAnalysisNE::ReloadShaders() {
    shader = {ShaderObject::FromFile(screen_shader_location, GL_VERTEX_SHADER),
              ShaderObject::FromFile(pca_shader_location, GL_FRAGMENT_SHADER)};
}