#include "d2nt.hpp"
#include <imgui.h>

/* #region GRADIENT FILTER*/
GradientFilter::GradientFilter(glm::ivec2 outputres,
                               std::string screen_vertex_shader,
                               std::string gradient_fragment_shader)
    : Renderstep(
          {ShaderObject::FromFile(screen_vertex_shader, GL_VERTEX_SHADER),
           ShaderObject::FromFile(gradient_fragment_shader,
                                  GL_FRAGMENT_SHADER)}) {
    inputTextures.emplace_back("input");
    inputTextures.emplace_back("laplacian");

    gradTex = std::make_shared<VirtualTexture>("gradient");
    outputTextures.push_back(gradTex);

    ResizeOwnedTextures(outputres);
}

GradientFilter::~GradientFilter() {}

std::string GradientFilter::name() const { return "Gradient"; }

void GradientFilter::SetInputTexture(std::shared_ptr<VirtualTexture> tex) {
    inputTextures[0].Set(tex);
}

void GradientFilter::SetLaplacianTexture(std::shared_ptr<VirtualTexture> tex) {
    inputTextures[1].Set(tex);
}

void GradientFilter::ResizeOwnedTextures(glm::ivec2 newres) {
    gradTex->assign(std::make_shared<Texture2D>(newres.x, newres.y, GL_RG32F));
    fbo = std::unique_ptr<FrameBufferObject>(new FrameBufferObject(
        {{GL_COLOR_ATTACHMENT0, *gradTex->getCurrent()}}));
}

void GradientFilter::Draw(CalibrationData &curCalib, Camera &curCam) {
    if (!inputTextures[0].IsSet())
        return;

    shader.Use();
    shader.SetTexture("inputTex", 0, *inputTextures[0].GetTex());
    shader.SetTexture("laplacian", 1, *inputTextures[1].GetTex());
    shader.SetUniform("resolution", curCalib.resolution_px);
    shader.SetUniform("discontinuity_aware", (int)dag);

    fbo->Bind();
    // viewport size already set

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void GradientFilter::DrawGui() { ImGui::Checkbox("discontinuity aware", &dag); }
/* #endregion */

/* #region LAPLACIAN FILTER */
LaplacianFilter::LaplacianFilter(glm::ivec2 outputres,
                                 std::string screen_vertex_shader,
                                 std::string gradient_fragment_shader)
    : Renderstep(
          {ShaderObject::FromFile(screen_vertex_shader, GL_VERTEX_SHADER),
           ShaderObject::FromFile(gradient_fragment_shader,
                                  GL_FRAGMENT_SHADER)},
          true) {
    inputTextures.emplace_back("input");

    lapTex = std::make_shared<VirtualTexture>("laplacian");
    outputTextures.push_back(lapTex);

    ResizeOwnedTextures(outputres);

    vert_loc = screen_vertex_shader;
    frag_loc = gradient_fragment_shader;
}

LaplacianFilter::~LaplacianFilter() {}

void LaplacianFilter::ReloadShaders() {
    shader = {ShaderObject::FromFile(frag_loc, GL_VERTEX_SHADER),
              ShaderObject::FromFile(vert_loc, GL_FRAGMENT_SHADER)};
}

std::string LaplacianFilter::name() const { return "Laplacian"; }

void LaplacianFilter::SetInputTexture(std::shared_ptr<VirtualTexture> tex) {
    inputTextures[0].Set(tex);
}

void LaplacianFilter::ResizeOwnedTextures(glm::ivec2 newres) {
    lapTex->assign(std::make_shared<Texture2D>(newres.x, newres.y, GL_RG32F));
    fbo = std::unique_ptr<FrameBufferObject>(
        new FrameBufferObject({{GL_COLOR_ATTACHMENT0, *lapTex->getCurrent()}}));
}

void LaplacianFilter::Draw(CalibrationData &curCalib, Camera &curCam) {
    if (!inputTextures[0].IsSet())
        return;

    shader.Use();
    shader.SetTexture("inputTex", 0, *inputTextures[0].GetTex());
    shader.SetUniform("resolution", curCalib.resolution_px);

    fbo->Bind();
    // viewport size already set

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void LaplacianFilter::DrawGui() {}

/* #endregion */

/* #region DEPTH TO NORMAL TRANSLATOR */
DepthToNormalTranslator::DepthToNormalTranslator(
    glm::ivec2 outputres, std::string screen_vertex_shader,
    std::string d2nt_fragment_shader)
    : Renderstep(
          {ShaderObject::FromFile(screen_vertex_shader, GL_VERTEX_SHADER),
           ShaderObject::FromFile(d2nt_fragment_shader, GL_FRAGMENT_SHADER)}) {
    inputTextures.emplace_back("depth / pos");
    inputTextures.emplace_back("gradient");

    normTex = std::make_shared<VirtualTexture>("d2nt normal");
    outputTextures.push_back(normTex);

    ResizeOwnedTextures(outputres);
}

DepthToNormalTranslator::~DepthToNormalTranslator() {}

std::string DepthToNormalTranslator::name() const { return "D2NT"; }

void DepthToNormalTranslator::SetDepthTexture(
    std::shared_ptr<VirtualTexture> tex) {
    inputTextures[0].Set(tex);
}

void DepthToNormalTranslator::SetGradientTexture(
    std::shared_ptr<VirtualTexture> tex) {
    inputTextures[1].Set(tex);
}

void DepthToNormalTranslator::ResizeOwnedTextures(glm::ivec2 newres) {
    normTex->assign(
        std::make_shared<Texture2D>(newres.x, newres.y, GL_RGBA32F));
    fbo = std::unique_ptr<FrameBufferObject>(new FrameBufferObject(
        {{GL_COLOR_ATTACHMENT0, *normTex->getCurrent()}}));
}

void DepthToNormalTranslator::Draw(CalibrationData &curCalib, Camera &curCam) {
    if (!inputTextures[0].IsSet() || !inputTextures[1].IsSet())
        return;

    shader.Use();
    shader.SetTexture("depth", 0, *inputTextures[0].GetTex());
    shader.SetTexture("gradient", 1, *inputTextures[1].GetTex());
    shader.SetUniform("fx", curCalib.fx);
    shader.SetUniform("fy", curCalib.fy);
    shader.SetUniform("c", curCalib.center_px);
    shader.SetUniform("res", curCalib.resolution_px);

    fbo->Bind();
    // viewport size already set

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void DepthToNormalTranslator::DrawGui() {}
/* #endregion */

/* #region MRF REFINER */
MRFRefiner::MRFRefiner(glm::ivec2 outputres, std::string screen_vertex_shader,
                       std::string mrf_fragment_shader)
    : Renderstep(
          {ShaderObject::FromFile(screen_vertex_shader, GL_VERTEX_SHADER),
           ShaderObject::FromFile(mrf_fragment_shader, GL_FRAGMENT_SHADER)}) {
    inputTextures.emplace_back("normals");
    inputTextures.emplace_back("laplacians");

    normTex = std::make_shared<VirtualTexture>("mrf normal");
    outputTextures.push_back(normTex);

    ResizeOwnedTextures(outputres);
}

MRFRefiner::~MRFRefiner() {}

std::string MRFRefiner::name() const { return "MRF Refiner"; }

void MRFRefiner::SetNormalTexture(std::shared_ptr<VirtualTexture> tex) {
    inputTextures[0].Set(tex);
}

void MRFRefiner::SetLaplacianTexture(std::shared_ptr<VirtualTexture> tex) {
    inputTextures[1].Set(tex);
}

void MRFRefiner::ResizeOwnedTextures(glm::ivec2 newres) {
    normTex->assign(
        std::make_shared<Texture2D>(newres.x, newres.y, GL_RGBA32F));
    fbo = std::unique_ptr<FrameBufferObject>(new FrameBufferObject(
        {{GL_COLOR_ATTACHMENT0, *normTex->getCurrent()}}));
}

void MRFRefiner::Draw(CalibrationData &curCalib, Camera &curCam) {
    if (!inputTextures[0].IsSet() || !inputTextures[1].IsSet())
        return;

    shader.Use();
    shader.SetTexture("normals", 0, *inputTextures[0].GetTex());
    shader.SetTexture("laplacian", 1, *inputTextures[1].GetTex());
    shader.SetUniform("resolution", curCalib.resolution_px);
    shader.SetUniform("pass", (int)passthrough);

    fbo->Bind();
    // viewport size already set

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void MRFRefiner::DrawGui() { ImGui::Checkbox("passthrough", &passthrough); }
/* #endregion */