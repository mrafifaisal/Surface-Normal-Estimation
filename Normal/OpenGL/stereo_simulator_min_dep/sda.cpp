#include "sda.hpp"
#include <imgui.h>

SpatialDiscontinuityAwareSNE::SpatialDiscontinuityAwareSNE(
    glm::ivec2 outputres, std::string screen_vertex_shader,
    std::string sda_init_fragment_shader, std::string sda_step_fragment_shader,
    std::string cp2tv_fragment_shader)
    : Renderstep(
          {ShaderObject::FromFile(screen_vertex_shader, GL_VERTEX_SHADER),
           ShaderObject::FromFile(sda_init_fragment_shader,
                                  GL_FRAGMENT_SHADER)},
          true),
      stepShader(
          {ShaderObject::FromFile(screen_vertex_shader, GL_VERTEX_SHADER),
           ShaderObject::FromFile(sda_step_fragment_shader,
                                  GL_FRAGMENT_SHADER)}),
      cp2tvShader(
          {ShaderObject::FromFile(screen_vertex_shader, GL_VERTEX_SHADER),
           ShaderObject::FromFile(cp2tv_fragment_shader, GL_FRAGMENT_SHADER)}),
      screen_shader_location(screen_vertex_shader),
      sda_init_shader_location(sda_init_fragment_shader),
      sda_step_shader_location(sda_step_fragment_shader),
      cp2tv_shader_location(cp2tv_fragment_shader) {
    inputTextures.emplace_back("laplacian");
    inputTextures.emplace_back("depth / pos");
    inputTextures.emplace_back("test gradient");

    gradAndEnergyTex1 = std::make_shared<VirtualTexture>("ZGrad + E (1)");
    gradAndEnergyTex2 = std::make_shared<VirtualTexture>("ZGrad + E (2)");
    minindexTex1 = std::make_shared<VirtualTexture>("min idx (1)");
    minindexTex2 = std::make_shared<VirtualTexture>("min idx (2)");
    normalTex = std::make_shared<VirtualTexture>("SDA normal");

    final_gradAndEnergy = std::make_shared<VirtualTexture>("ZGrad + E (final)");
    final_minindex = std::make_shared<VirtualTexture>("min idx (final)");

    outputTextures.push_back(normalTex);
    outputTextures.push_back(final_gradAndEnergy);
    outputTextures.push_back(final_minindex);

    ResizeOwnedTextures(outputres);
}

SpatialDiscontinuityAwareSNE::~SpatialDiscontinuityAwareSNE() {}

std::string SpatialDiscontinuityAwareSNE::name() const { return "SDA-SNE"; }

void SpatialDiscontinuityAwareSNE::ResizeOwnedTextures(glm::ivec2 newres) {
    gradAndEnergyTex1->assign(std::make_shared<Texture2D>(
        newres.x, newres.y, GL_RGBA32F)); // xy: gradient, zw: energy
    minindexTex1->assign(std::make_shared<Texture2D>(
        newres.x, newres.y, GL_RG8I,
        GL_RG_INTEGER)); // Potential error: source format is float and cannot
                         // be set from here
    fbo1 = std::unique_ptr<FrameBufferObject>(new FrameBufferObject(
        {{GL_COLOR_ATTACHMENT0, *gradAndEnergyTex1->getCurrent()},
         {GL_COLOR_ATTACHMENT1, *minindexTex1->getCurrent()}}));

    gradAndEnergyTex2->assign(
        std::make_shared<Texture2D>(newres.x, newres.y, GL_RGBA32F));
    minindexTex2->assign(std::make_shared<Texture2D>(newres.x, newres.y,
                                                     GL_RG8I, GL_RG_INTEGER));
    fbo2 = std::unique_ptr<FrameBufferObject>(new FrameBufferObject(
        {{GL_COLOR_ATTACHMENT0, *gradAndEnergyTex2->getCurrent()},
         {GL_COLOR_ATTACHMENT1, *minindexTex2->getCurrent()}}));

    normalTex->assign(
        std::make_shared<Texture2D>(newres.x, newres.y, GL_RGBA32F));
    normalFbo = std::unique_ptr<FrameBufferObject>(new FrameBufferObject(
        {{GL_COLOR_ATTACHMENT0, *normalTex->getCurrent()}}));
}

void SpatialDiscontinuityAwareSNE::Draw(CalibrationData &curCalib,
                                        Camera &curCam) {
    if (!inputTextures[0].IsSet() || !inputTextures[1].IsSet())
        return;

    shader.Use(); // Initialization
    shader.SetTexture("laplacian", 0, *inputTextures[0].GetTex());
    shader.SetTexture("depth", 1, *inputTextures[1].GetTex());
    shader.SetUniform("resolution", curCalib.resolution_px);

    fbo1->Bind();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // iterations
    for (int i = 0; i < sda_iters; ++i) {
        stepShader.Use();
        stepShader.SetTexture("laplacian", 0, *inputTextures[0].GetTex());
        stepShader.SetTexture("depth", 1, *inputTextures[1].GetTex());
        stepShader.SetUniform("resolution", curCalib.resolution_px);
        stepShader.SetUniform("iter_idx",
                              i + 1); // compatibility with matlab idx

        if (i % 2 == 0) {
            fbo2->Bind();
            stepShader.SetTexture("prevGE", 2,
                                  *gradAndEnergyTex1->getCurrent());
            stepShader.SetTexture("prev_idx", 3, *minindexTex1->getCurrent());
        } else {
            fbo1->Bind();
            stepShader.SetTexture("prevGE", 2,
                                  *gradAndEnergyTex2->getCurrent());
            stepShader.SetTexture("prev_idx", 3, *minindexTex2->getCurrent());
        }

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    normalFbo->Bind();
    cp2tvShader.Use();

    cp2tvShader.SetTexture("depth", 0, *inputTextures[1].GetTex());
    if (sda_iters % 2 == 0) {
        // use tex 2
        cp2tvShader.SetTexture("depthGradient", 1,
                               *gradAndEnergyTex1->getCurrent());
        final_gradAndEnergy->assign(gradAndEnergyTex1->getCurrent()); // final grad and energy
        final_minindex->assign(minindexTex1->getCurrent()); // final
    } else {
        // use tex 1
        cp2tvShader.SetTexture("depthGradient", 1,
                               *gradAndEnergyTex2->getCurrent());
        final_gradAndEnergy->assign(gradAndEnergyTex2->getCurrent()); // final grad and energy
        final_minindex->assign(minindexTex2->getCurrent()); // final
    }
    if(inputTextures[2].IsSet() && test_gradient)
        cp2tvShader.SetTexture("depthGradient", 1, *inputTextures[2].GetTex());
    cp2tvShader.SetUniform("k11", curCalib.fx);
    cp2tvShader.SetUniform("k22", curCalib.fy);
    cp2tvShader.SetUniform("resolution", curCalib.resolution_px);
    cp2tvShader.SetUniform("principle_point", curCalib.center_px);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void SpatialDiscontinuityAwareSNE::DrawGui() {
    ImGui::InputInt("iterations", &sda_iters);
    ImGui::Checkbox("use test gradient", &test_gradient);
    if(test_gradient && !inputTextures[2].IsSet())
        ImGui::Text("Warning: test gradient is not set");
}

void SpatialDiscontinuityAwareSNE::SetLaplacianTexture(
    std::shared_ptr<VirtualTexture> tex) {
    inputTextures[0].Set(tex);
}

void SpatialDiscontinuityAwareSNE::SetDepthTexture(
    std::shared_ptr<VirtualTexture> tex) {
    inputTextures[1].Set(tex);
}

void SpatialDiscontinuityAwareSNE::ReloadShaders() {
    shader = {
        ShaderObject::FromFile(screen_shader_location, GL_VERTEX_SHADER),
        ShaderObject::FromFile(sda_init_shader_location, GL_FRAGMENT_SHADER)};
    stepShader = {
        ShaderObject::FromFile(screen_shader_location, GL_VERTEX_SHADER),
        ShaderObject::FromFile(sda_step_shader_location, GL_FRAGMENT_SHADER)};
    cp2tvShader = {
        ShaderObject::FromFile(screen_shader_location, GL_VERTEX_SHADER),
        ShaderObject::FromFile(cp2tv_shader_location, GL_FRAGMENT_SHADER)};
}