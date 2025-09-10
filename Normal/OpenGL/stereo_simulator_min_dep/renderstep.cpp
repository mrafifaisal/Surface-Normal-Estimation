#include "renderstep.hpp"
#include "camera.hpp"

#include <imgui.h>
#include <iostream>

Renderstep::Renderstep(ProgramObject shader, bool shaderReload)
    : shader(shader), shaderReloadable(shaderReload) {}

Renderstep::~Renderstep() {}

void Renderstep::Render(CalibrationData &curCalib, Camera &curCam) {
    if (measurePerformance) {
        GLuint qobj;
        glGenQueries(1, &qobj);
        glBeginQuery(GL_TIME_ELAPSED, qobj);
        Draw(curCalib, curCam);
        glEndQuery(GL_TIME_ELAPSED);
        glGetQueryObjectui64v(qobj, GL_QUERY_RESULT, &lastGpuTimeNs);
        glDeleteQueries(1, &qobj);
    } else {
        Draw(curCalib, curCam);
    }
}

void Renderstep::ReloadShaders() {
    std::cerr << "Warning: " << name()
              << " doesn't support on the fly shader reloading.\n";
}

void Renderstep::DrawGui() {}

void Renderstep::ResizeOwnedTextures(glm::ivec2 newres) {}

std::vector<TextureSlot> &Renderstep::GetInputs() { return inputTextures; }
const std::vector<std::shared_ptr<VirtualTexture>> &
Renderstep::GetOutputs() const {
    return outputTextures;
}

u_int64_t Renderstep::GetLastGpuTimeNs() const { return lastGpuTimeNs; }

PointCloudVisualizer::PointCloudVisualizer(std::string pointcloud_shader_path,
                                           std::string color_shader_path)
    : Renderstep(ProgramObject(
          {ShaderObject::FromFile(pointcloud_shader_path, GL_VERTEX_SHADER),
           ShaderObject::FromFile(color_shader_path, GL_FRAGMENT_SHADER)})) {
    inputTextures.emplace_back("positions");
    inputTextures.emplace_back("colors");
}

PointCloudVisualizer::~PointCloudVisualizer() {}

std::string PointCloudVisualizer::name() const {
    return "Point cloud visualizer";
}

void PointCloudVisualizer::SetPositionsInputTexture(
    std::shared_ptr<VirtualTexture> tex) {
    inputTextures[0].Set(tex);
}

void PointCloudVisualizer::SetColorsInputTexture(
    std::shared_ptr<VirtualTexture> tex) {
    inputTextures[1].Set(tex);
}

PointCloudVisualizer::PointCloudVisualizer()
    : PointCloudVisualizer("shaders/modular/pointcloud.vert",
                           "shaders/modular/texturecolor.frag") {}

void PointCloudVisualizer::Draw(CalibrationData &curCalib, Camera &curCam) {
    shader.Use();
    shader.SetTexture("positions", 0, *inputTextures[0].GetTex());
    shader.SetTexture("colors", 1, *inputTextures[1].GetTex());
    shader.SetUniform("MVP", curCam.GetVP());
    shader.SetUniform("nth", nth);
    shader.SetUniform("size", pointSize);

    // GLint res[4];
    // glGetIntegerv(GL_VIEWPORT, res);

    shader.SetUniform("resolution", curCalib.resolution_px);

    // default fbo already used
    // viewport size already set

    glDrawArrays(GL_POINTS, 0,
                 curCalib.resolution_px.x * curCalib.resolution_px.y);
}

void PointCloudVisualizer::DrawGui() {
    ImGui::InputInt2("display only nth", &nth[0]);
    ImGui::DragFloat("point size", &pointSize);
}

std::string Triangulation::name() const { return "Triangulation"; }

Triangulation::Triangulation(glm::ivec2 outputres,
                             std::string screen_vertex_shader,
                             std::string triangulation_fragment_shader)
    : Renderstep(ProgramObject(
          {ShaderObject::FromFile(screen_vertex_shader, GL_VERTEX_SHADER),
           ShaderObject::FromFile(triangulation_fragment_shader,
                                  GL_FRAGMENT_SHADER)}), true),
      vertex_shader(screen_vertex_shader),
      fragment_shader(triangulation_fragment_shader) {
    inputTextures.emplace_back("disparities");
    posTex = std::make_shared<VirtualTexture>("triangulated pos");
    outputTextures.push_back(posTex);

    ResizeOwnedTextures(outputres);
}

Triangulation::~Triangulation() {}

void Triangulation::SetDisparitiesInputTexture(
    std::shared_ptr<VirtualTexture> tex) {
    inputTextures[0].Set(tex);
}

void Triangulation::ResizeOwnedTextures(glm::ivec2 newres) {
    posTex->assign(std::make_shared<Texture2D>(newres.x, newres.y, GL_RGB32F));
    fbo = std::unique_ptr<FrameBufferObject>(
        new FrameBufferObject({{GL_COLOR_ATTACHMENT0, *posTex->getCurrent()}}));
}

void Triangulation::Draw(CalibrationData &curCalib, Camera &curCam) {
    shader.Use();
    shader.SetTexture("disparities", 0, *inputTextures[0].GetTex());

    // GLint res[4];
    // glGetIntegerv(GL_VIEWPORT, res);

    // shader.SetUniform("resolution", curCalib.resolution_px);
    shader.SetUniform("b", curCalib.baseline_mm / 1000.0f);
    shader.SetUniform("fx", curCalib.fx);
    shader.SetUniform("fy", curCalib.fy);
    shader.SetUniform("res", curCalib.resolution_px);
    shader.SetUniform("c", curCalib.center_px);
    shader.SetUniform("scale", scale);
    shader.SetUniform("doffs", curCalib.doffs);

    fbo->Bind();
    // viewport size already set?

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void Triangulation::DrawGui() { ImGui::DragFloat("scale", &scale, 10.0f); }

void Triangulation::ReloadShaders() {
    shader = {ShaderObject::FromFile(vertex_shader, GL_VERTEX_SHADER),
              ShaderObject::FromFile(fragment_shader, GL_FRAGMENT_SHADER)};
}