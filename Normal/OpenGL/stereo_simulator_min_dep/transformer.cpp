#include "renderstep.hpp"
#include <imgui.h>

Transformer::Transformer(glm::ivec2 outputres, std::string screen_vertex_shader,
                         std::string transform_fragment_shader)
    : Renderstep(
          {ShaderObject::FromFile(screen_vertex_shader, GL_VERTEX_SHADER),
           ShaderObject::FromFile(transform_fragment_shader,
                                  GL_FRAGMENT_SHADER)}) {
    inputTextures.emplace_back("base");

    transformedTex = std::make_shared<VirtualTexture>("transformed texture");
    outputTextures.push_back(transformedTex);

    ResizeOwnedTextures(outputres);
}

Transformer::~Transformer() {}

void Transformer::SetBaseTexture(std::shared_ptr<VirtualTexture> tex) {
    inputTextures[0].Set(tex);
}

std::string Transformer::name() const { return "Transform"; }

void Transformer::ResizeOwnedTextures(glm::ivec2 newres) {
    transformedTex->assign(
        std::make_shared<Texture2D>(newres.x, newres.y, GL_RGBA32F));
    fbo = std::unique_ptr<FrameBufferObject>(new FrameBufferObject(
        {{GL_COLOR_ATTACHMENT0, *transformedTex->getCurrent()}}));
}

void Transformer::Draw(CalibrationData &curCalib, Camera &curCam) {
    if (!inputTextures[0].IsSet())
        return;

    shader.Use();
    shader.SetTexture("base", 0, *inputTextures[0].GetTex());
    shader.SetUniform("flipX", (int)flipX);
    shader.SetUniform("flipY", (int)flipY);
    shader.SetUniform("multiply", multiply);
    shader.SetUniform("color_transform", color_transform);
    shader.SetUniform("color_shift", color_shift);
    shader.SetUniform("normalize3", (int)normalize_out3);

    fbo->Bind();
    // viewport size already set

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void Transformer::DrawGui() {
    ImGui::Checkbox("flip X", &flipX);
    ImGui::SameLine();
    ImGui::Checkbox("flip Y", &flipY);
    ImGui::DragFloat("multiply with", &multiply);

    ImGui::Text("Color transform matrix:");
    glm::mat4 colmat_t = glm::transpose(color_transform);
    for (int i = 0; i < 4; ++i) {
        ImGui::PushID(i);
        ImGui::InputFloat4("", &colmat_t[i][0]);
        ImGui::PopID();
    }
    color_transform = glm::transpose(colmat_t);
    ImGui::InputFloat4("color shift", &color_shift[0]);
    
    ImGui::Checkbox("Normalize XYZ", &normalize_out3);
}