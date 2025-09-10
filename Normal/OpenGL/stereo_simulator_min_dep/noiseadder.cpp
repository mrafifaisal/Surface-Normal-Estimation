#include "renderstep.hpp"
#include <imgui.h>

NoiseAdder::NoiseAdder(glm::ivec2 outputres, std::string screen_vertex_shader,
                       std::string noise_fragment_shader)
    : Renderstep(
          {ShaderObject::FromFile(screen_vertex_shader, GL_VERTEX_SHADER),
           ShaderObject::FromFile(noise_fragment_shader, GL_FRAGMENT_SHADER)}) {
    inputTextures.emplace_back("base");

    noisyTex = std::make_shared<VirtualTexture>("noisy texture");
    outputTextures.push_back(noisyTex);

    noiseTex = std::make_shared<VirtualTexture>("noise");
    outputTextures.push_back(noiseTex);

    ResizeOwnedTextures(outputres);
}

NoiseAdder::~NoiseAdder() {}

void NoiseAdder::SetBaseTexture(std::shared_ptr<VirtualTexture> tex) {
    inputTextures[0].Set(tex);
}

std::string NoiseAdder::name() const { return "Noise adder"; }

void NoiseAdder::ResizeOwnedTextures(glm::ivec2 newres) {
    noisyTex->assign(std::make_shared<Texture2D>(newres.x, newres.y, GL_R32F));
    fbo = std::unique_ptr<FrameBufferObject>(new FrameBufferObject(
        {{GL_COLOR_ATTACHMENT0, *noisyTex->getCurrent()}}));
    noiseTex->assign(GenerateNoiseTexture(newres.x, newres.y, stddeviation));
}

std::shared_ptr<Texture2D>
NoiseAdder::GenerateNoiseTexture(int w, int h, float noise_standard_deviation) {
    std::normal_distribution<float> dist(0, noise_standard_deviation);

    auto tex = std::make_shared<Texture2D>(w, h, GL_R32F);

    std::vector<float> values(w * h);
    for (int i = 0; i < w; ++i) {
        for (int j = 0; j < h; ++j) {
            values[i * h + j] = dist(rng);
        }
    }
    tex->UploadData(GL_RED, GL_FLOAT, w, h, (void *)values.data());
    tex->SetFiltering(GL_NEAREST, GL_NEAREST);
    return tex;
}

void NoiseAdder::Draw(CalibrationData &curCalib, Camera &curCam) {
    if (need_regen || realtime){
        need_regen = false;
        noiseTex->assign(GenerateNoiseTexture(
            curCalib.resolution_px.x, curCalib.resolution_px.y, stddeviation));
    }

    if (!inputTextures[0].IsSet())
        return;

    shader.Use();
    shader.SetTexture("base", 0, *inputTextures[0].GetTex());
    shader.SetTexture("noise", 1, *noiseTex->getCurrent());
    shader.SetUniform("pass", (int)passthrough);

    fbo->Bind();
    // viewport size already set

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void NoiseAdder::DrawGui() {
    if (ImGui::InputFloat("stddev", &stddeviation, 0.01f, 1.0f) && stddeviation > 0) {
        need_regen = true;
    }

    ImGui::Checkbox("realtime", &realtime);
    ImGui::SameLine();
    ImGui::Checkbox("passthrough", &passthrough);
}