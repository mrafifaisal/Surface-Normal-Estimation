#include "renderstep.hpp"
#include <imgui.h>

TextureComparison::TextureComparison(glm::ivec2 outputres,
                                     std::string screen_vertex_shader,
                                     std::string comparison_fragment_shader)
    : Renderstep(
          {ShaderObject::FromFile(screen_vertex_shader, GL_VERTEX_SHADER),
           ShaderObject::FromFile(comparison_fragment_shader,
                                  GL_FRAGMENT_SHADER)},
          true),
      vert_shader(screen_vertex_shader),
      frag_shader(comparison_fragment_shader) {
    inputTextures.emplace_back("reference");
    inputTextures.emplace_back("observed");
    inputTextures.emplace_back("mask");

    diffTex = std::make_shared<VirtualTexture>("difference");
    outputTextures.push_back(diffTex);

    ResizeOwnedTextures(outputres);
}

TextureComparison::~TextureComparison() {}

void TextureComparison::SetComparisonTexture(
    std::shared_ptr<VirtualTexture> tex, bool reference) {
    inputTextures[reference ? 0 : 1].Set(tex);
}

void TextureComparison::SetMaskTexture(std::shared_ptr<VirtualTexture> tex) {
    inputTextures[2].Set(tex);
}

std::string TextureComparison::name() const { return "Texture compare"; }

void TextureComparison::ResizeOwnedTextures(glm::ivec2 newres) {
    diffTex->assign(std::make_shared<Texture2D>(newres.x, newres.y, GL_RGB32F));
    fbo = std::unique_ptr<FrameBufferObject>(new FrameBufferObject(
        {{GL_COLOR_ATTACHMENT0, *diffTex->getCurrent()}}));
}

void TextureComparison::Draw(CalibrationData &curCalib, Camera &curCam) {
    if (!inputTextures[0].IsSet() || !inputTextures[1].IsSet())
        return;

    shader.Use();
    shader.SetTexture("reference", 0, *inputTextures[0].GetTex());
    shader.SetTexture("observed", 1, *inputTextures[1].GetTex());
    shader.SetTexture("mask", 2, *inputTextures[2].GetTex());
    shader.SetUniform("useMask", (int)useMask);
    shader.SetUniform("error_type", (int)mode);
    shader.SetUniform("gradient_map", (int)useGradient);

    fbo->Bind();
    // viewport size already set

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void TextureComparison::DrawGui() {
    ImGui::Combo("Mode", (int *)&mode, option_names, 4);
    ImGui::Checkbox("Mask", &useMask);
    ImGui::Checkbox("Gradient map", &useGradient);
}

void TextureComparison::ReloadShaders() {
    shader = {ShaderObject::FromFile(vert_shader, GL_VERTEX_SHADER),
              ShaderObject::FromFile(frag_shader, GL_FRAGMENT_SHADER)};
}