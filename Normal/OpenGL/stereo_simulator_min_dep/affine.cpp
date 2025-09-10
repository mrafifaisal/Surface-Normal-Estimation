#include "renderstep.hpp"
#include <assert.h>
#include <imgui.h>

AffineParameterEstimator::AffineParameterEstimator(
    glm::ivec2 outputres, std::string screen_vertex_shader,
    std::string affine_fragment_shader)
    : Renderstep(
          {ShaderObject::FromFile(screen_vertex_shader, GL_VERTEX_SHADER),
           ShaderObject::FromFile(affine_fragment_shader, GL_FRAGMENT_SHADER)},
          true),
      screen_shader(screen_vertex_shader),
      affine_shader(affine_fragment_shader) {
    inputTextures.emplace_back("disparities");
    inputTextures.emplace_back("edges");
    inputTextures.emplace_back("depth");
    affTex = std::make_shared<VirtualTexture>("affine params");
    outputTextures.push_back(affTex);

    ResizeOwnedTextures(outputres);
}

AffineParameterEstimator::~AffineParameterEstimator() {}

std::string AffineParameterEstimator::name() const {
    return "Affine parameter estimator";
}

void AffineParameterEstimator::SetDisparitiesInputTexture(
    std::shared_ptr<VirtualTexture> tex) {
    inputTextures[0].Set(tex);
}

void AffineParameterEstimator::SetEdgesInputTexture(
    std::shared_ptr<VirtualTexture> tex) {
    inputTextures[1].Set(tex);
}

void AffineParameterEstimator::SetDepthInputTexture(
    std::shared_ptr<VirtualTexture> tex) {
    inputTextures[2].Set(tex);
}

void AffineParameterEstimator::ResizeOwnedTextures(glm::ivec2 newres) {
    affTex->assign(std::make_shared<Texture2D>(newres.x, newres.y, GL_RG32F));
    fbo = std::unique_ptr<FrameBufferObject>(
        new FrameBufferObject({{GL_COLOR_ATTACHMENT0, *affTex->getCurrent()}}));
}

void AffineParameterEstimator::Draw(CalibrationData &curCalib, Camera &curCam) {
    if (kernels_invalid) {
        if (mode == 1) {
            kernels_invalid = false;
            RecomputeAffineLSQConvFilter(glm::pi<float>() / 2, 0.1f,
                                         curCalib.resolution_px.x /
                                             ((float)curCalib.resolution_px.y),
                                         curCalib.resolution_px);
        } else if (mode >= 2) {
            kernels_invalid = false;
            RecomputeAdaptiveDirs();
        }
    }

    shader.Use();
    shader.SetTexture("disparities", 0, *inputTextures[0].GetTex());
    if(mode == 2 || mode == 5)
        shader.SetTexture("weights", 1, *inputTextures[1].GetTex());
    shader.SetTexture("depth", 2, *inputTextures[2].GetTex());
    shader.SetUniform("resolution", curCalib.resolution_px);
    shader.SetUniform("mode", mode);
    shader.SetUniform("conv_size", (int)v.size());
    shader.SetUniform("v", v.data(), v.size());
    shader.SetUniform("conv1", conv1.data(), conv1.size());
    shader.SetUniform("conv2", conv2.data(), conv2.size());
    shader.SetUniform("adaptive_simple_thresh", adaptive_simple_thresh);
    shader.SetUniform("adaptive_cumulative_thresh", adaptive_cumulative_thresh);
    shader.SetUniform("adaptive_dir_count", adaptive_dir_count);
    shader.SetUniform("adaptive_dirs", adaptive_dirs.data(),
                      adaptive_dirs.size());
    shader.SetUniform("adaptive_max_step_count", adaptive_max_step_count);
    shader.SetUniform("adaptive_depth_thresh_ratio",
                      adaptive_depth_thresh_ratio);
    shader.SetUniform("costhresh", glm::cos(glm::radians(test_deg_thresh)));

    fbo->Bind();
    // viewport size already set

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void AffineParameterEstimator::DrawGui() {
    ImGui::RadioButton("basic", &mode, 0);
    ImGui::SameLine();
    if (ImGui::RadioButton("lsq", &mode, 1))
        kernels_invalid = true;
    ImGui::SameLine();
    if(ImGui::RadioButton("threshold", &mode, 5))
        kernels_invalid = true;

    if (ImGui::RadioButton("cumulative", &mode, 2))
        kernels_invalid = true;
    ImGui::SameLine();
    if (ImGui::RadioButton("covered depth", &mode, 3))
        kernels_invalid = true;
    ImGui::SameLine();
    if (ImGui::RadioButton("angle", &mode, 4))
        kernels_invalid = true;
    
    ImGui::PushID(1);
    if (mode == 2 || mode == 3 || mode == 4 || mode == 5) {
        if (mode == 2)
            ImGui::InputFloat("cumulative threshold", &adaptive_cumulative_thresh);
        if (mode == 3)
            ImGui::InputFloat("depth threshold ratio",
                              &adaptive_depth_thresh_ratio);
        if (mode == 4){
            ImGui::InputFloat("Test degree threshold", &test_deg_thresh);
        }
        if(mode == 5) {
            ImGui::InputFloat("threshold", &adaptive_simple_thresh);
        }
        ImGui::InputInt("max steps", &adaptive_max_step_count);
        
        if (ImGui::InputInt("directions", &adaptive_dir_count)) {
            if (adaptive_dir_count < 0)
                adaptive_dir_count = 0;
            if (adaptive_dir_count > 32)
                adaptive_dir_count = 32;
            kernels_invalid = true;
        }
    }
    ImGui::PopID();
    // if (ImGui::Checkbox("lsq", &enable_lsq)) {
    //     kernels_invalid = true;
    // }
    if (ImGui::InputInt("kernel size", &conv_size, 2)) {
        if (conv_size % 2 == 0)
            conv_size += 1;
        if (conv_size > 15)
            conv_size = 15;
        else if (conv_size < 1)
            conv_size = 1;
        kernels_invalid = true;
    }
}

void AffineParameterEstimator::ReloadShaders() {
    shader = {ShaderObject::FromFile(screen_shader, GL_VERTEX_SHADER),
              ShaderObject::FromFile(affine_shader, GL_FRAGMENT_SHADER)};
}

void AffineParameterEstimator::RecomputeAffineLSQConvFilter(float fovy, float f,
                                                            float aspect,
                                                            glm::vec2 res) {
    assert(conv_size <= 15); // Important: update shader when changed
    
    v.clear();
    v.reserve(conv_size * conv_size);

    // float h = f * tan(fovy / 2);
    // float w = aspect * h;

    // float spx = 2 * w * 1.0f / res.x;
    // float spy = 2 * h * 1.0f / res.y;

    for (int x = -conv_size / 2; x <= conv_size / 2; ++x) {
        for (int y = -conv_size / 2; y <= conv_size / 2; ++y) {
            // v.push_back(glm::vec2(x * spx, y * spy));
            // if(glm::length(glm::vec2(x,y)) <= conv_size/2.0f) // uncomment for circular kernel
                v.push_back(glm::vec2(x, y));
        }
    }

    conv1.resize(v.size());
    conv2.resize(v.size());

    float a = 0, b = 0, c = 0;
    for (int i = 0; i < v.size(); ++i) {
        a += v[i].x * v[i].x;
        b += v[i].x * v[i].y;
        c += v[i].y * v[i].y;
    }

    float inv = 1 / (a * c - b * b);
    for (int i = 0; i < v.size(); ++i) {
        conv1[i] = inv * (v[i].x * c - v[i].y * b);
        conv2[i] = inv * (-v[i].x * b + v[i].y * a);
    }

    // send to shader without scaling:
    // for (auto &v : v) {
    //     v.x /= spx;
    //     v.y /= spy;
    // }
}

void AffineParameterEstimator::RecomputeAdaptiveDirs() {
    assert(adaptive_dir_count <= 32); // Important: update shader when changed
    adaptive_dirs.resize(adaptive_dir_count);
    for (int i = 0; i < adaptive_dir_count; ++i) {
        float ang = i * glm::pi<float>() * 2 / adaptive_dir_count;
        adaptive_dirs[i] = glm::vec2(cosf(ang), sinf(ang));
    }
}