#include "renderstep.hpp"
#include <imgui.h>

TextureViewer::TextureViewer(std::string screen_vertex_shader,
                             std::string view_fragment_shader)
    : Renderstep(
          {ShaderObject::FromFile(screen_vertex_shader, GL_VERTEX_SHADER),
           ShaderObject::FromFile(view_fragment_shader, GL_FRAGMENT_SHADER)}) {
    inputTextures.emplace_back("texture");
}

TextureViewer::~TextureViewer() {}

std::string TextureViewer::name() const { return "Texture viewer"; }

void TextureViewer::SetDisplayedTexture(std::shared_ptr<VirtualTexture> tex) {
    inputTextures[0].Set(tex);
}

void TextureViewer::Draw(CalibrationData& curCalib, Camera& curCam) {
    if(!inputTextures[0].IsSet())
        return;

    shader.Use();
    shader.SetTexture("tex", 0, *inputTextures[0].GetTex());

    // fbo & viewport already set

    glDisable(GL_DEPTH_TEST);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glEnable(GL_DEPTH_TEST);
}