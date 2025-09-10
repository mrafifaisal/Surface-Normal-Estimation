#include "app.hpp"
#include "canny.hpp"
#include "d2nt.hpp"
#include "date.h"
#include "pca.hpp"
#include "sda.hpp"
#include <chrono>
#include <fstream>
#include <functional>
#include <glm/gtc/type_ptr.hpp>
#include <initializer_list>
#include <iomanip>
#include <iostream>
#include <memory>
#include <nfd.h>
#include <sstream>
#include <utility>

App::App()
    : cubeBuffer(CubeVertices), cubeVao(),
      calibSim(
          300, {800, 600}, cam1.cliplPlanes.x, cam1.fovy,
          cam1.aspect), //(50, {200, 150}, {400, 300}, 0.1f * 200, 0.1f * 150)
      simColProgram(
          {ShaderObject::FromFile("shaders/vert.vert", GL_VERTEX_SHADER),
           ShaderObject::FromFile("shaders/frag.frag", GL_FRAGMENT_SHADER)}),
      simDispProgram(
          {ShaderObject::FromFile("shaders/disparity.vert", GL_VERTEX_SHADER),
           ShaderObject::FromFile("shaders/disparity.frag",
                                  GL_FRAGMENT_SHADER)}),
      sphereProgram({ShaderObject::FromFile("shaders/modular/screen.vert",
                                            GL_VERTEX_SHADER),
                     ShaderObject::FromFile("shaders/disparity_sphere.frag",
                                            GL_FRAGMENT_SHADER)}),
      testmesh("models/pyramid.obj"), sphereMesh("models/sphere.obj", true),
      bunnyMesh("models/bunny.obj"), avgGpuTime(10) {
    auto resx = calibSim.resolution_px.x;
    auto resy = calibSim.resolution_px.y;

    col1Sim = std::make_shared<VirtualTexture>(
        "sim col 1", std::make_shared<Texture2D>(resx, resy, GL_RGB8));
    col2Sim = std::make_shared<VirtualTexture>(
        "sim col 2", std::make_shared<Texture2D>(resx, resy, GL_RGB8));
    depth1Sim = std::make_shared<VirtualTexture>(
        "sim depth 1",
        std::make_shared<Texture2D>(resx, resy, GL_DEPTH_COMPONENT24,
                                    GL_DEPTH_COMPONENT));
    depth2Sim = std::make_shared<VirtualTexture>(
        "sim depth 2",
        std::make_shared<Texture2D>(resx, resy, GL_DEPTH_COMPONENT24,
                                    GL_DEPTH_COMPONENT));
    posSim = std::make_shared<VirtualTexture>(
        "sim pos", std::make_shared<Texture2D>(resx, resy, GL_RGB32F));
    normSim = std::make_shared<VirtualTexture>(
        "sim norm",
        std::make_shared<Texture2D>(
            resx, resy,
            GL_RGBA32F)); // Alpha not used, but needed because imageLoad only
                          // supports either rg or rgba :(
    dispSim = std::make_shared<VirtualTexture>(
        "sim disp", std::make_shared<Texture2D>(resx, resy, GL_R32F));

    maskSim = std::make_shared<VirtualTexture>(
        "sim mask",
        std::make_shared<Texture2D>(resx, resy, GL_RG32I, GL_RG_INTEGER));

    simulationTextures = {col1Sim, col2Sim, depth1Sim, depth2Sim,
                          posSim,  normSim, dispSim,   maskSim};

    fboDispSim = std::unique_ptr<FrameBufferObject>(new FrameBufferObject(
        {{GL_COLOR_ATTACHMENT0, *col1Sim->getCurrent()},
         {GL_COLOR_ATTACHMENT1, *dispSim->getCurrent()},
         {GL_COLOR_ATTACHMENT2, *posSim->getCurrent()},
         {GL_COLOR_ATTACHMENT3, *normSim->getCurrent()},
         {GL_COLOR_ATTACHMENT4, *maskSim->getCurrent()},
         {GL_DEPTH_ATTACHMENT, *depth1Sim->getCurrent()}}));

    fboColSim = std::unique_ptr<FrameBufferObject>(new FrameBufferObject(
        {{GL_COLOR_ATTACHMENT0, *col2Sim->getCurrent()},
         {GL_DEPTH_ATTACHMENT, *depth2Sim->getCurrent()}}));

    colLoad = std::make_shared<VirtualTexture>("col load");
    dispLoad = std::make_shared<VirtualTexture>("disp load");
    normLoad = std::make_shared<VirtualTexture>("norm load");
    maskLoad = std::make_shared<VirtualTexture>("mask load");

    loadedTextures = {colLoad, dispLoad, normLoad, maskLoad};

    cubeVao.AddAttribute(cubeBuffer, 0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                         0);
    cubeVao.AddAttribute(cubeBuffer, 1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                         sizeof(glm::vec3));
    cubeVao.AddAttribute(cubeBuffer, 2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                         sizeof(glm::vec3) * 2);

    controlledCamera = &freeCam;
    freeCam.MoveTo(glm::vec3(0, 0, 0));
    freeCam.LookAt(glm::vec3(0, 0, 1));

    auto transformer = std::make_shared<Transformer>(calibSim.resolution_px);
    firstTransformStep = transformer;
    transformer->SetBaseTexture(dispSim);
    auto transTex = transformer->GetOutputs()[0];
    intermediateSteps.push_back(transformer);
    transformstep = transformer;

    auto transformer2 = std::make_shared<Transformer>(calibSim.resolution_px);
    transformer2->SetBaseTexture(normLoad);
    intermediateSteps.push_back(transformer2);
    transformer2->enabled = false;
    transformstep2 = transformer2;

    noisestep = std::make_shared<NoiseAdder>(calibSim.resolution_px);
    noisestep->SetBaseTexture(transTex);
    auto noisyTex = noisestep->GetOutputs()[0];
    intermediateSteps.push_back(noisestep);

    auto triangulatestep =
        std::make_shared<Triangulation>(calibSim.resolution_px);
    triangulatestep->SetDisparitiesInputTexture(noisyTex);
    auto &posTri = triangulatestep->GetOutputs()[0];
    intermediateSteps.push_back(triangulatestep);

    auto cannystep =
        std::make_shared<CannyEdgeDetector>(calibSim.resolution_px);
    cannystep->SetTexture(posTri);
    intermediateSteps.push_back(cannystep);

    auto lapstep = std::make_shared<LaplacianFilter>(calibSim.resolution_px);
    lapstep->SetInputTexture(posTri);
    intermediateSteps.push_back(lapstep);

    auto gradstep = std::make_shared<GradientFilter>(calibSim.resolution_px);
    gradstep->SetInputTexture(posTri);
    gradstep->SetLaplacianTexture(lapstep->GetOutputs()[0]);
    intermediateSteps.push_back(gradstep);

    affinestep =
        std::make_shared<AffineParameterEstimator>(calibSim.resolution_px);
    affinestep->SetDisparitiesInputTexture(noisyTex);
    affinestep->SetEdgesInputTexture(lapstep->GetOutputs()[0]);
    affinestep->SetDepthInputTexture(posTri);
    intermediateSteps.push_back(affinestep);

    // auto affinestep2 =
    //     std::make_shared<AffineParameterEstimator>(calibSim.resolution_px);
    // affinestep2->SetDisparitiesInputTexture(dispSim);
    // intermediateSteps.push_back(affinestep2);

    auto cne_step =
        std::make_shared<CrossNormalEstimator>(calibSim.resolution_px);
    cne_step->SetPositionsInputTexture(posTri);
    intermediateSteps.push_back(cne_step);

    auto affine_normal_step =
        std::make_shared<AffineNormalEstimator>(calibSim.resolution_px);
    affine_normal_step->SetAffineParameterInputTexture(
        affinestep->GetOutputs()[0]);
    affine_normal_step->SetPositionsInputTexture(posTri);
    intermediateSteps.push_back(affine_normal_step);
    auto &normAff = affine_normal_step->GetOutputs()[0];

    auto d2ntstep =
        std::make_shared<DepthToNormalTranslator>(calibSim.resolution_px);
    d2ntstep->SetDepthTexture(posTri);
    d2ntstep->SetGradientTexture(gradstep->GetOutputs()[0]);
    intermediateSteps.push_back(d2ntstep);

    auto sdastep =
        std::make_shared<SpatialDiscontinuityAwareSNE>(calibSim.resolution_px);
    sdastep->SetDepthTexture(posTri);
    sdastep->SetLaplacianTexture(lapstep->GetOutputs()[0]);
    intermediateSteps.push_back(sdastep);

    pcarenderstep =
        std::make_shared<PrincipalComponentAnalysisNE>(calibSim.resolution_px);
    pcarenderstep->SetPositionTexture(posTri);
    intermediateSteps.push_back(pcarenderstep);

    auto mrfstep = std::make_shared<MRFRefiner>(calibSim.resolution_px);
    mrfstep->SetNormalTexture(d2ntstep->GetOutputs()[0]);
    mrfstep->SetLaplacianTexture(lapstep->GetOutputs()[0]);
    intermediateSteps.push_back(mrfstep);

    auto texturecompare =
        std::make_shared<TextureComparison>(calibSim.resolution_px);
    texturecompare->SetComparisonTexture(normSim, true);
    texturecompare->SetComparisonTexture(normAff, false);
    texturecompare->SetMaskTexture(maskSim);
    intermediateSteps.push_back(texturecompare);

    measureDifferenceStep =
        std::make_shared<MeasureDifference>(calibSim.resolution_px);
    intermediateSteps.push_back(measureDifferenceStep);

    textureSaver = std::make_shared<SaveTexture>();

    auto cloudstep = std::make_shared<PointCloudVisualizer>();
    pointCloudRenderStep = cloudstep;
    cloudstep->SetPositionsInputTexture(posTri);
    cloudstep->SetColorsInputTexture(col1Sim);
    displaySteps.push_back(cloudstep);

    auto normstep = std::make_shared<NormalVisualizer>();
    normstep->SetPositionsInputTexture(posTri);
    normstep->SetNormalsInputTexture(normAff);
    normstep->SetMasksInputTexture(maskSim);
    displaySteps.push_back(normstep);

    auto textureview = std::make_shared<TextureViewer>();
    displaySteps.push_back(textureview);

    auto &io = ImGui::GetIO();
    io.Fonts->AddFontDefault();
    overlayfont = io.Fonts->AddFontFromFileTTF("dejavu-sans.bold.ttf", 60);
}

void App::Update() {
    static int last_tick = 0;
    float delta = (SDL_GetTicks() - last_tick) / 1000.0f;

    float ctrl_coeff_h = d_down - a_down;
    float ctrl_coeff_v = w_down - s_down;

    float speed = delta * move_sensitivity;
    if (shift_down)
        speed *= 0.1;
    if (ctrl_down)
        speed *= 10;

    controlledCamera->MoveTo(controlledCamera->pos +
                             controlledCamera->GetRight() * ctrl_coeff_h *
                                 speed);
    controlledCamera->MoveTo(controlledCamera->pos +
                             controlledCamera->GetFWD() * ctrl_coeff_v * speed);

    last_tick = SDL_GetTicks();
}

void App::Render() {
    glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
    glClearColor(background_color.r, background_color.g, background_color.b,
                 background_color.a);
    glLineWidth(3.0f);

    GLuint qobj;
    if (measureOnlyTotalGPU) {
        // corrupts per step measurements -> disabled by default
        glGenQueries(1, &qobj);
        glBeginQuery(GL_TIME_ELAPSED, qobj);
    }

    cam2.MoveTo(cam1.pos + cam1.GetRight() * calibSim.baseline_mm / 1000.0f);
    cam2.at = cam2.pos + cam1.at - cam1.pos;

    glViewport(0, 0, calibSim.resolution_px.x, calibSim.resolution_px.y);

    ClearMask();
    fboDispSim->Bind();
    simDispProgram.Use();
    simDispProgram.SetUniform("rightVP", cam2.GetVP());
    simDispProgram.SetUniform("VP", cam1.GetVP());
    simDispProgram.SetUniform("V", cam1.GetV());
    // simDispProgram.SetUniform("f", cam1.cliplPlanes.x);
    simDispProgram.SetUniform("resx", calibSim.resolution_px.x);
    // simDispProgram.SetUniform("aspect", cam1.aspect);
    // simDispProgram.SetUniform("fovy", cam1.fovy);
    DrawScene(simDispProgram, cam1);
    fboColSim->Bind();
    DrawScene(simColProgram, cam2);

    // Moved to separate "scene"
    // fboDispSim->Bind();
    // sphereProgram.Use();
    // sphereProgram.SetUniform("rightVP", cam2.GetVP());
    // sphereProgram.SetUniform("VP", cam1.GetVP());
    // sphereProgram.SetUniform("invVP", glm::inverse(cam1.GetVP()));
    // sphereProgram.SetUniform("V", cam1.GetV());
    // sphereProgram.SetUniform("resx", calibSim.resolution_px.x);
    // sphereProgram.SetUniform("cam_pos", cam1.pos);

    // sphereProgram.SetUniform("center", glm::vec3(6.0f, 2.0f, -7.0f));
    // sphereProgram.SetUniform("radius", 3.0f);
    // glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    auto curCalib = (use_simulation ? calibSim : *calibLoad);

    if (!use_simulation && loadResOverride) {
        curCalib.resolution_px *= resOverride;
        curCalib.fx *= resOverride.x;
        curCalib.fy *= resOverride.y;
        curCalib.center_px *= resOverride;
    }

    glViewport(0, 0, curCalib.resolution_px.x, curCalib.resolution_px.y);

    for (auto &step : intermediateSteps) {
        if (step->enabled) {
            if (measureOnlyTotalGPU) {
                bool tmp = step->measurePerformance;
                step->measurePerformance = false;
                step->Render(curCalib, freeCam);
                step->measurePerformance = tmp;
            } else
                step->Render(curCalib, freeCam);
        }
    }

    auto panelCam = freeCam;
    int cur_win_h = half_vert_res ? win_h / 2 : win_h;
    if (two_panel_mode && cur_win_h > 0) {
        panelCam.aspect = ((float)win_w / 2) / cur_win_h;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for (auto &step : displaySteps) {
        if (step->enabled) {
            if (two_panel_mode) {
                if (step->RenderToOtherPanel())
                    glViewport(0, 0, win_w / 2, cur_win_h);
                else
                    glViewport(win_w / 2, 0, win_w / 2, cur_win_h);
            } else
                glViewport(0, 0, win_w, cur_win_h);
            if (measureOnlyTotalGPU) {
                bool tmp = step->measurePerformance;
                step->measurePerformance = false;
                step->Render(curCalib, panelCam);
                step->measurePerformance = tmp;
            } else
                step->Render(curCalib, panelCam);
        }
    }

    glViewport(0, 0, win_w, win_h);

    if (measureOnlyTotalGPU) {
        glEndQuery(GL_TIME_ELAPSED);
        glGetQueryObjectui64v(qobj, GL_QUERY_RESULT, &totalGpuTimeNs);
        glDeleteQueries(1, &qobj);

        avgGpuTime.Slide(totalGpuTimeNs);
    }
}

void App::DrawGUI() {

    {
        static float delay = 5.0f;
        static float duration = 3.0f;
        float inter_value = 0;
        static float *fvalue = nullptr;
        static int *ivalue = nullptr;
        static float ends[2] = {0.0f, 1.0f};
        static int selected_target = 1;
        static int type_mode = 0; // 0 float, 1 int
        const char *type_names[2] = {"Float", "Int"};
        const char *target_names[6] = {"None",           "Noise stddev",
                                       "Affine kernel",  "PCA kernel",
                                       "adaptive steps", "adaptive dirs"};
        static void *target_ptrs[6] = {nullptr,
                                       &noisestep->stddeviation,
                                       &affinestep->conv_size,
                                       &pcarenderstep->size,
                                       &affinestep->adaptive_max_step_count,
                                       &affinestep->adaptive_dir_count};
        static glm::ivec2 window_size_gui = {1920, 540};
        static char text_prefix[100] = "";
        static ImVec2 overlay_pos = {win_w / 2, win_h - 80};
        static ImVec2 overlay_size = {200, 75};

        float time = SDL_GetTicks() / 1000.0f - recording_start;
        if (fvalue != nullptr || ivalue != nullptr) {
            // ImGui::InputFloat("value", value);
            float t = (time - delay) / duration;
            if (t < 0)
                inter_value = ends[0];
            else if (t > 1)
                inter_value = ends[1];
            else {
                inter_value = glm::mix(ends[0], ends[1], t);
            }

            if (type_mode == 0)
                *fvalue = inter_value;
            else if (type_mode == 1) {
                if (selected_target == 2 || selected_target == 3) {
                    *ivalue = ((int)(inter_value / 2.0f)) * 2 + 1;
                } else
                    *ivalue = (int)inter_value;
            }
            if (selected_target == 1)
                noisestep->need_regen = true; // yikes
            else if (selected_target == 2)
                affinestep->kernels_invalid = true; // yies #2
            else if (selected_target == 5)
                affinestep->RecomputeAdaptiveDirs();
        }
        if (show_gui) {
            if (ImGui::Begin("Animation")) {
                std::string time_str = std::to_string(time);
                ImGui::Text(time_str.c_str());

                ImGui::InputFloat("delay", &delay);
                ImGui::InputFloat("duration", &duration);
                ImGui::InputFloat2("range", ends);
                ImGui::Combo("type", &type_mode, type_names, 2);
                if (ImGui::Combo("target", &selected_target, target_names, 6)) {
                    if (type_mode == 0)
                        fvalue = (float *)target_ptrs[selected_target];
                    else if (type_mode == 1)
                        ivalue = (int *)target_ptrs[selected_target];
                }

                if (type_mode == 0 && fvalue != nullptr)
                    ImGui::InputFloat("value", fvalue);
                if (type_mode == 1 && ivalue != nullptr)
                    ImGui::InputInt("value", ivalue);

                if (ImGui::Button("start now")) {
                    recording_start = SDL_GetTicks() / 1000.0f;
                }

                ImGui::InputInt2("window size to force",
                                 glm::value_ptr(window_size_gui));
                if (ImGui::Button("set window size")) {
                    SDL_SetWindowSize(SDL_GL_GetCurrentWindow(),
                                      window_size_gui.x, window_size_gui.y);
                }

                ImGui::InputText("label", text_prefix, 100);
                ImGui::DragFloat2("overlay pos", &overlay_pos.x);
                ImGui::DragFloat2("overlay size", &overlay_size.x);
            }
            ImGui::End();
        }

        if ((type_mode == 0 && fvalue != nullptr ||
             type_mode == 1 && ivalue != nullptr) &&
            show_animated_value_overlay) {
            ImGuiWindowFlags window_flags =
                ImGuiWindowFlags_NoDecoration |
                ImGuiWindowFlags_AlwaysAutoResize |
                ImGuiWindowFlags_NoSavedSettings |
                ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
            ImGui::SetNextWindowBgAlpha(0.5f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 25.0f);
            // ImGui::PushStyleColor(ImGuiCol_WindowBg,
            // ImVec4(0.3f,0.3f,0.3f,1.0f));
            ImGui::SetNextWindowPos(overlay_pos, ImGuiCond_Always,
                                    ImVec2(0.5, 0.5));
            // ImGui::SetNextWindowSize(overlay_size);
            bool p_open = true;

            if (ImGui::Begin("overlay", &p_open, window_flags)) {
                std::stringstream str("");
                str.clear();
                str << text_prefix;
                str << std::fixed;
                str.precision(3); // << std::setw(6);
                if (type_mode == 0)
                    str << std::setw(6) << *fvalue;
                else if (type_mode == 1)
                    str << std::setw(2) << *ivalue;
                ImGui::PushFont(overlayfont);
                ImGui::Text(str.str().c_str());
                ImGui::PopFont();
            }
            ImGui::End();
            ImGui::PopStyleVar(2);
            // ImGui::PopStyleColor();
        }
    }

    if (!show_gui)
        return;

    // ImGui::ShowDemoWindow();

    if (ImGui::Begin("Data")) {
        if (ImGui::RadioButton("simulation", (int *)&use_simulation, (int)true))
            SwitchMode(false);
        ImGui::SameLine();
        if (ImGui::RadioButton("loaded disparities", (int *)&use_simulation,
                               (int)false))
            SwitchMode(true);

        ImGui::Checkbox("comparison mode", &two_panel_mode);
        ImGui::Checkbox("half screen mode", &half_vert_res);
        ImGui::SliderFloat("camera speed", &move_sensitivity, 3.0f, 50.0f);
        float cam_fovy = freeCam.fovy;
        if (ImGui::SliderAngle("camera fovy", &cam_fovy, 20.0f, 90.0f))
            freeCam.SetFovY(cam_fovy);
        if (ImGui::Button("reset free cam")) {
            freeCam.MoveTo(glm::vec3(0, 0, 0));
            freeCam.LookAt(glm::vec3(0, 0, 1));
        }
        ImGui::ColorEdit4("background", &background_color[0]);

        if (ImGui::BeginTabBar("Mode specific settings")) {
            if (ImGui::BeginTabItem("Simulation")) {
                ImGui::Combo("Scene", &selected_scene, scene_names,
                             scene_count);
                if (selected_scene == 0) { // 3 meshes
                    ImGui::SeparatorText("Rotating object");
                    ImGui::DragFloat("rotation speed", &rotationSpeed, 0.1f);
                    ImGui::DragFloat("rotation offset", &rotationOffset, 1);
                    ImGui::DragFloat("height", &height, 0.01f);
                }
                if(selected_scene == 1)
                    ImGui::DragFloat("rotation offset", &rotationOffset, 1);
                ImGui::SeparatorText("Virtual Camera");
                if (ImGui::InputInt2("stereo resolution",
                                     &calibSim.resolution_px[0])) {
                    ResizeStereoCameraFbos(calibSim.resolution_px);
                    cam1.aspect = ((float)calibSim.resolution_px.x) /
                                  calibSim.resolution_px.y;
                    cam2.aspect = cam1.aspect;
                    calibSim = CalibrationData(
                        calibSim.baseline_mm, calibSim.resolution_px,
                        cam1.cliplPlanes.x, cam1.fovy, cam1.aspect);
                    if (use_simulation) {
                        for (auto &step : intermediateSteps) {
                            step->ResizeOwnedTextures(calibSim.resolution_px);
                        }
                    }
                }
                ImGui::DragFloat("baseline", &calibSim.baseline_mm, 0.05f);
                static bool curCamStereo = (controlledCamera == &cam1);
                if (ImGui::Checkbox("Control stereo camera", &curCamStereo)) {
                    if (curCamStereo)
                        controlledCamera = &cam1;
                    else {
                        controlledCamera = &freeCam;
                    }
                }

                static glm::vec3 pos = cam1.pos, at = cam1.at;
                if (ImGui::DragFloat3("position", glm::value_ptr(pos))) {
                }

                if (ImGui::DragFloat3("target", glm::value_ptr(at))) {
                }

                if (ImGui::Button("set!")) {
                    cam1.MoveTo(pos);
                    cam1.LookAt(at);
                }

                ImGui::Text("%s", calibSim.ToString().c_str());

                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Loaded")) {
                static std::string disparity_file = "<none>";
                ImGui::Text("loaded disp: %s", disparity_file.c_str());
                if (ImGui::Button("load disparities")) {
                    nfdchar_t *outPath;
                    nfdresult_t result =
                        NFD_OpenDialog(&outPath, nullptr, 0, nullptr);
                    if (result == NFD_OKAY) {
                        disparity_file = outPath;
                        dispLoad->assign(std::make_shared<Texture2D>(outPath));
                        NFD_FreePath(outPath);
                    }
                }
                static std::string calibration_file = "<none>";
                ImGui::Text("loaded calib: %s", calibration_file.c_str());
                if (calibLoad != nullptr)
                    ImGui::Text("%s", calibLoad->ToString().c_str());
                if (ImGui::Button("load calibration data")) {
                    nfdchar_t *outPath;
                    nfdresult_t result =
                        NFD_OpenDialog(&outPath, nullptr, 0, nullptr);
                    if (result == NFD_OKAY) {
                        calibration_file = outPath;
                        calibLoad = std::make_unique<CalibrationData>(outPath);
                        NFD_FreePath(outPath);
                    }
                }
                static std::string color_file = "<none>";
                ImGui::Text("loaded colors: %s", color_file.c_str());
                if (ImGui::Button("load colors")) {
                    nfdchar_t *outPath;
                    nfdresult_t result =
                        NFD_OpenDialog(&outPath, nullptr, 0, nullptr);
                    if (result == NFD_OKAY) {
                        color_file = outPath;
                        colLoad->assign(std::make_shared<Texture2D>(outPath));
                        NFD_FreePath(outPath);
                    }
                }
                static std::string normal_file = "<none>";
                ImGui::Text("loaded normals: %s", normal_file.c_str());
                if (ImGui::Button("load normals")) {
                    nfdchar_t *outPath;
                    nfdresult_t result =
                        NFD_OpenDialog(&outPath, nullptr, 0, nullptr);
                    if (result == NFD_OKAY) {
                        normal_file = outPath;
                        normLoad->assign(
                            std::make_shared<Texture2D>(outPath, true));
                        NFD_FreePath(outPath);
                    }
                }

                static std::string mask_file = "<none>";
                ImGui::Text("loaded mask: %s", mask_file.c_str());
                if (ImGui::Button("load mask")) {
                    nfdchar_t *outPath;
                    nfdresult_t result =
                        NFD_OpenDialog(&outPath, nullptr, 0, nullptr);
                    if (result == NFD_OKAY) {
                        mask_file = outPath;
                        maskLoad->assign(std::make_shared<Texture2D>(outPath));
                        NFD_FreePath(outPath);
                    }
                }

                ImGui::Checkbox("Override resolution", &loadResOverride);
                if (loadResOverride)
                    ImGui::InputFloat2("new resolution",
                                       glm::value_ptr(resOverride));

                if (ImGui::Button("Load example KITTI")) {
                    disparity_file =
                        "tests/kitti/"
                        "1_disp.png";
                    calibration_file = "tests/kitti/kitti15.calib";
                    color_file = "tests/kitti/1_col.png";
                    dispLoad->assign(
                        std::make_shared<Texture2D>(disparity_file));
                    calibLoad =
                        std::make_unique<CalibrationData>(calibration_file);
                    colLoad->assign(std::make_shared<Texture2D>(color_file));
                    transformstep->multiply = 333;
                    SwitchMode(true);
                    
                    // loadedDisparityCoeff = -0.0147870370918;
                }
                if (ImGui::Button("Load example MIDDLEBURY")) {
                    disparity_file =
                        "tests/"
                        "middlebury/disp0.pfm";
                    calibration_file = "tests/middlebury/"
                                       "middlebury.calib";
                    color_file = "tests/middlebury/im0.png";
                    dispLoad->assign(
                        std::make_shared<Texture2D>(disparity_file));
                    calibLoad =
                        std::make_unique<CalibrationData>(calibration_file);
                    colLoad->assign(std::make_shared<Texture2D>(color_file));
                    // loadedDisparityCoeff = -0.0147870370918;
                    transformstep->multiply = 250;
                    SwitchMode(true);
                }
                if (ImGui::Button("Load example irs")) {
                    disparity_file =
                        "tests/"
                        "irs/d_1.pfm";
                    calibration_file = "tests/irs/irs.calib";
                    color_file = "tests/irs/l_1.png";
                    normal_file = "tests/irs/n_1.exr";
                    dispLoad->assign(
                        std::make_shared<Texture2D>(disparity_file));
                    calibLoad =
                        std::make_unique<CalibrationData>(calibration_file);
                    colLoad->assign(std::make_shared<Texture2D>(color_file));
                    normLoad->assign(
                        std::make_shared<Texture2D>(normal_file, true));
                    // loadedDisparityCoeff = -0.0147870370918;
                    SwitchMode(true);
                    transformstep2->color_transform[0][0] = -2;
                    transformstep2->color_transform[3][0] = 1;
                    transformstep2->color_transform[1][1] = -2;
                    transformstep2->color_transform[3][1] = 1;
                    transformstep2->color_transform[2][2] = 2;
                    transformstep2->color_transform[3][2] = -1;
                }
                if (ImGui::Button("Load example Cityscapes")) {
                    disparity_file =
                        "tests/cityscapes/"
                        "berlin_000000_000019_disparity.png";
                    calibration_file = "tests/cityscapes/cityscapes.calib";
                    color_file = "tests/cityscapes/berlin_000000_000019_leftImg8bit.png";
                    dispLoad->assign(
                        std::make_shared<Texture2D>(disparity_file));
                    calibLoad =
                        std::make_unique<CalibrationData>(calibration_file);
                    colLoad->assign(std::make_shared<Texture2D>(color_file));
                    transformstep->multiply = 333;
                    SwitchMode(true);
                    
                    // loadedDisparityCoeff = -0.0147870370918;
                }

                // ImGui::InputFloat("Disparity coefficient: ",
                //                   &loadedDisparityCoeff);
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }
    ImGui::End();

    if (ImGui::Begin("Rendersteps")) {
        if (ImGui::BeginTabBar("Stages")) {
            if (ImGui::BeginTabItem("Intermediate steps")) {
                int idx = 0;
                for (auto &step : intermediateSteps) {
                    ImGui::PushID(idx++);
                    IntermediateStepMenu(step);
                    ImGui::Separator();
                    ImGui::PopID();
                }
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Display steps")) {
                int idx = 0;
                for (auto &step : displaySteps) {
                    ImGui::PushID(idx++);
                    ImGui::Checkbox("", &(step->enabled));
                    ImGui::SameLine();
                    ImGui::Text(step->name().c_str());
                    int idx2 = 0;
                    for (auto &input : step->GetInputs()) {
                        ImGui::PushID(idx2++);

                        if (ImGui::BeginMenu(input.name_with_content.c_str())) {
                            SwitchTexture(input);

                            ImGui::EndMenu();
                        }
                        ImGui::PopID();
                    }
                    step->DrawGui();
                    ImGui::Separator();
                    ImGui::PopID();
                }
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }
    ImGui::End();

    if (ImGui::Begin("Measurements")) {
        ImGui::PushID("measure");
        IntermediateStepMenu(measureDifferenceStep);
        ImGui::PopID();
        ImGui::PushID("save");
        IntermediateStepMenu(textureSaver);
        ImGui::PopID();

        ImGui::Checkbox("measure only total GPU", &measureOnlyTotalGPU);
        if (measureOnlyTotalGPU) {
            if (avgGpuTime.Ready()) {
                double us = avgGpuTime.GetAverage() / 1000.0;
                ImGui::Text("Total GPU time: %f us", us);
            } else {
                ImGui::Text("Measuring total GPU time");
            }
            int window_size = avgGpuTime.GetSize();
            if (ImGui::InputInt("average frames", &window_size)) {
                avgGpuTime.Resize(window_size);
            }
        }

        MeasurePerformance();
    }
    ImGui::End();
}

void App::IntermediateStepMenu(std::shared_ptr<Renderstep> step) {
    ImGui::Checkbox("", &(step->enabled));
    if (step->shaderReloadable) {
        ImGui::SameLine();
        if (ImGui::Button("Reload")) {
            std::cout << "Attempting to reload shader for " << step->name()
                      << "\n";
            step->ReloadShaders();
            std::cout << "Shader " << step->name() << " reloaded\n";
        }
    }
    ImGui::SameLine();

    if (ImGui::CollapsingHeader(step->name().c_str())) {
        // ImGui::Text(step->name().c_str());

        ImGui::Checkbox("measure", &step->measurePerformance);
        if (step->measurePerformance) {
            ImGui::SameLine();
            double us = step->GetLastGpuTimeNs() / 1000.0;
            ImGui::Text("%f us", us);
        }
        int idx2 = 0;
        for (auto &input : step->GetInputs()) {
            ImGui::PushID(idx2++);

            if (ImGui::BeginMenu(input.name_with_content.c_str())) {
                SwitchTexture(input);

                ImGui::EndMenu();
            }
            ImGui::PopID();
        }
        step->DrawGui();
    }
}

void App::SwitchTexture(TextureSlot &slot) {
    if (ImGui::BeginMenu("simulation textures")) {
        for (auto &simtex : simulationTextures) {
            bool isSelected = slot.Get() == simtex;
            if (ImGui::MenuItem(simtex->displayName.c_str(), nullptr,
                                &isSelected)) {
                slot.Set(simtex);
            }
        }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("loaded textures")) {
        for (auto &loadtex : loadedTextures) {
            bool isSelected = slot.Get() == loadtex;
            if (ImGui::MenuItem(loadtex->displayName.c_str(), nullptr,
                                &isSelected)) {
                slot.Set(loadtex);
            }
        }
        ImGui::EndMenu();
    }

    for (auto &step : intermediateSteps) {
        if (ImGui::BeginMenu(step->name().c_str())) {
            for (auto &output : step->GetOutputs()) {
                bool isSelected = slot.Get() == output;
                if (ImGui::MenuItem(output->displayName.c_str(), nullptr,
                                    &isSelected)) {
                    slot.Set(output);
                }
            }
            ImGui::EndMenu();
        }
    }
}

void App::MeasurePerformance() {
    static std::string export_path = "perf.csv";
    static std::ofstream file;
    static int measured_frames = 0;
    static int sample_count = 30;
    static bool measuring = false;
    ImGui::Text(export_path.c_str());
    ImGui::SameLine();
    if (ImGui::Button("select")) {
        nfdchar_t *outPath;
        nfdresult_t result =
            NFD_SaveDialog(&outPath, nullptr, 0, nullptr, nullptr);
        if (result == NFD_OKAY) {
            export_path = outPath;
            NFD_FreePath(outPath);
        }
    }

    if (measuring) {
        ImGui::Text("Measuring: %i/%i", measured_frames, sample_count);
    } else {
        ImGui::InputInt("sample count", &sample_count);
        if (ImGui::Button("Start")) {
            auto &curCalib = (use_simulation ? calibSim : *calibLoad);
            file.open(export_path, std::ios_base::openmode::_S_app);
            std::string renderer = (char *)glGetString(GL_RENDERER);
            file << date::format("%F %T", std::chrono::system_clock::now());
            file << " | " << renderer << " | " << curCalib.resolution_px.x
                 << "x" << curCalib.resolution_px.y << "\n";
            for (auto &step : intermediateSteps) {
                if (step->measurePerformance)
                    file << step->name() << ";";
            }
            file << "\n";
            measured_frames = 0;
            measuring = true;
        }
    }

    if (measuring && measured_frames < sample_count) {
        for (auto &step : intermediateSteps) {
            if (step->measurePerformance)
                file << step->GetLastGpuTimeNs() << ";";
        }
        file << "\n";
        ++measured_frames;
    }

    if (measuring && measured_frames == sample_count) {
        file.close();
        measuring = false;
    }
}

void App::DrawScene(ProgramObject &program, Camera &camera) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    std::invoke(scene_functions[selected_scene], *this, program, camera);
}

void App::DrawScene3Meshes(ProgramObject &program, Camera &camera) {
    float t = SDL_GetTicks() / 1000.0f;

    ClearMask();

    glm::mat4 vp = camera.GetVP();
    glm::mat4 world =
        glm::translate(glm::vec3(0.0f, height, 0.0f)) *
        glm::rotate(t * rotationSpeed + glm::radians(rotationOffset),
                    glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 VworldIT = glm::transpose(glm::inverse(camera.GetV() * world));
    glm::mat4 mvp = vp * world;

    program.Use();
    program.SetUniform("MVP", mvp);
    program.SetUniform("world", world);
    program.SetUniform("VworldIT", VworldIT);
    program.SetUniform("worldIT", glm::transpose(glm::inverse(world)));
    program.SetUniform("useColor", (int)false);
    program.SetUniform("cam_pos", camera.pos);

    // TODO
    // cubeVao.Bind();
    // glDrawArrays(GL_TRIANGLES, 0, 36);
    testmesh.Draw();

    // world = glm::translate(glm::vec3(6.0f, 2.0f, -7.0f)) *
    //         glm::scale(glm::vec3(3.0f, 3.0f, 3.0f));
    // VworldIT = glm::transpose(glm::inverse(camera.GetV() * world));
    // mvp = vp * world;
    // program.SetUniform("MVP", mvp);
    // program.SetUniform("world", world);
    // program.SetUniform("VworldIT", VworldIT);
    // program.SetUniform("worldIT", glm::transpose(glm::inverse(world)));
    // sphereMesh.Draw();

    world = glm::translate(glm::vec3(-6.0f, -1.0f, -10.0f)) *
            glm::scale(glm::vec3(40.0f, 40.0f, 40.0f)) *
            glm::rotate(glm::radians(230.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    mvp = vp * world;
    program.SetUniform("MVP", mvp);
    program.SetUniform("world", world);
    program.SetUniform("VworldIT",
                       glm::transpose(glm::inverse(camera.GetV() * world)));
    program.SetUniform("worldIT", glm::transpose(glm::inverse(world)));
    bunnyMesh.Draw();

    cubeVao.Bind();

    world = glm::translate(glm::vec3(0.0f, -1.0f, 0.0f)) *
            glm::scale(glm::vec3(1000.0f, 0.01f, 1000.0f));
    program.SetUniform("MVP", vp * world);
    program.SetUniform("world", world);
    program.SetUniform("VworldIT",
                       glm::transpose(glm::inverse(camera.GetV() * world)));
    program.SetUniform("worldIT", glm::transpose(glm::inverse(world)));
    glDrawArrays(GL_TRIANGLES, 0, 36);

    // corner 1
    world = glm::translate(glm::vec3(0.0f, 0.0f, -100.0f)) *
            glm::rotate(glm::radians(45.0f), glm::vec3(0, 1, 0)) *
            glm::scale(glm::vec3(10000.0f, 10000.0f, 1.0f));
    program.SetUniform("MVP", vp * world);
    program.SetUniform("world", world);
    program.SetUniform("VworldIT",
                       glm::transpose(glm::inverse(camera.GetV() * world)));
    program.SetUniform("worldIT", glm::transpose(glm::inverse(world)));
    program.SetUniform("useColor", (int)true);
    program.SetUniform("color", glm::vec3(0.75f, 0.5f, 0.0f));
    glDrawArrays(GL_TRIANGLES, 0, 36);

    // corner 2
    world = glm::translate(glm::vec3(0.0f, 0.0f, -100.0f)) *
            glm::rotate(glm::radians(-45.0f + 180), glm::vec3(0, 1, 0)) *
            glm::scale(glm::vec3(10000.0f, 10000.0f, 1.0f));
    program.SetUniform("MVP", vp * world);
    program.SetUniform("world", world);
    program.SetUniform("VworldIT",
                       glm::transpose(glm::inverse(camera.GetV() * world)));
    program.SetUniform("worldIT", glm::transpose(glm::inverse(world)));
    program.SetUniform("useColor", (int)true);
    program.SetUniform("color", glm::vec3(0.0f, 0.5f, 0.75f));
    glDrawArrays(GL_TRIANGLES, 0, 36);

    // Sphere (analytic)
    sphereProgram.Use();
    sphereProgram.SetUniform("rightVP", cam2.GetVP());
    sphereProgram.SetUniform("VP", cam1.GetVP());
    sphereProgram.SetUniform("invVP", glm::inverse(cam1.GetVP()));
    sphereProgram.SetUniform("V", cam1.GetV());
    sphereProgram.SetUniform("resx", calibSim.resolution_px.x);
    sphereProgram.SetUniform("cam_pos", cam1.pos);

    sphereProgram.SetUniform("center", glm::vec3(6.0f, 2.0f, -7.0f));
    sphereProgram.SetUniform("radius", 3.0f);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void App::DrawSceneStabilityTest(ProgramObject &program, Camera &camera) {
    float t = SDL_GetTicks() / 1000.0f;

    glm::mat4 vp = camera.GetVP();
    glm::mat4 world = glm::rotate(glm::radians(rotationOffset + 45.0f),
                                  glm::vec3(1.0f, 0.0f, 0.0f)) *
                      glm::scale(glm::vec3(1000000.0f, 0.1f, 1000000.0f));
    glm::mat4 VworldIT = glm::transpose(glm::inverse(camera.GetV() * world));
    glm::mat4 mvp = vp * world;

    program.Use();
    program.SetUniform("MVP", mvp);
    program.SetUniform("world", world);
    program.SetUniform("VworldIT", VworldIT);
    program.SetUniform("worldIT", glm::transpose(glm::inverse(world)));
    program.SetUniform("useColor", (int)false);
    program.SetUniform("cam_pos", camera.pos);

    cubeVao.Bind();
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

void App::DrawSceneSphere(ProgramObject &program, Camera &camera) {
    glm::mat4 vp = camera.GetVP();
    glm::mat4 world = // glm::translate(glm::vec3(0.0f, 0.0f, 1.0f)) *
        glm::scale(glm::vec3(1000.0f, 1000.0f, 1.0f));
    glm::mat4 VworldIT = glm::transpose(glm::inverse(camera.GetV() * world));
    glm::mat4 mvp = vp * world;

    program.Use();
    program.SetUniform("MVP", mvp);
    program.SetUniform("world", world);
    program.SetUniform("VworldIT", VworldIT);
    program.SetUniform("worldIT", glm::transpose(glm::inverse(world)));
    program.SetUniform("useColor", (int)false);
    program.SetUniform("cam_pos", camera.pos);

    cubeVao.Bind();
    glDrawArrays(GL_TRIANGLES, 0, 36);

    ClearMask();

    // Sphere
    sphereProgram.Use();
    sphereProgram.SetUniform("rightVP", cam2.GetVP());
    sphereProgram.SetUniform("VP", cam1.GetVP());
    sphereProgram.SetUniform("invVP", glm::inverse(cam1.GetVP()));
    sphereProgram.SetUniform("V", cam1.GetV());
    sphereProgram.SetUniform("resx", calibSim.resolution_px.x);
    sphereProgram.SetUniform("cam_pos", cam1.pos);

    sphereProgram.SetUniform("center", glm::vec3(0.0f, 0.0f, 2.0f));
    sphereProgram.SetUniform("radius", 1.4f);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void App::DrawSceneDiscontinuity(ProgramObject &program, Camera &camera) {
    ClearMask();

    // testmesh pyramid
    // cubeVao cube
    static std::vector<Transform> cubes = {
        {{0.0, -2.0, 0.0}, {0.0, 0.0, 0.0}, {1000.0, 1.0, 1000.0}},
        {{0.0, 0.0, -10.0}, {0.0, 0.0, 0.0}, {2.0, 2.0, 2.0}},
        {{2.0, -1.0, -7.0}, {-30.0, 0.0, 0.0}, {1.0, 1.0, 1.0}},
        {{10.0, 0.0, -10.0}, {0.0, 0.0, 0.0}, {2.0, 2.0, 2.0}},
        {{-10.0, 0.0, -10.0}, {0.0, 0.0, 0.0}, {2.0, 2.0, 2.0}},
        {{5.0, 0.0, -5.0}, {0.0, 45.0, 0.0}, {1.0, 1.0, 1.0}},
        {{-5.0, 0.0, -5.0}, {30.0, 45.0, 0.0}, {2.0, 1.0, 2.0}},
        {{3.0, -1.0, 0.0}, {10.0, 0.0, 0.0}, {1.0, 1.0, 1.0}},
        {{2.0, -1.0, 3.0}, {0.0, 70.0, 30.0}, {0.2, 0.5, 0.2}},
        {{2.0, -1.0, -1.0}, {0.0, 45.0, 0.0}, {0.5, 0.5, 0.5}},
    };
    static std::vector<Transform> pyramids = {
        {{-2.0, -1.0, -2.0}, {0.0, 30.0, 0.0}, {1.0, 1.5, 1.0}},
        {{-1.0, -1.0, -1.0}, {0.0, 50.0, 0.0}, {0.5, 1.0, 0.5}},
        {{0.0, -1.0, 0.0}, {0.0, 0.0, 0.0}, {0.25, 0.5, 0.25}},
        {{2.0, -1.0, 1.0}, {0.0, 0.0, 0.0}, {0.5, 0.5, 0.5}},
        // {{0.0, 0.0, -50.0}, {0.0, -45.0, 0.0}, {30.0, 20.0, 30.0}},
    };
    static std::vector<Sphere> spheres = {{{-1.0, -0.5, 3.5}, 0.3},
                                          {{0.0, -2.3, 2.25}, 1.5}};

    cubeVao.Bind();
    program.Use();
    program.SetUniform("useColor", (int)false);
    program.SetUniform("cam_pos", camera.pos);

    for (auto &c : cubes) {
        glm::mat4 world =
            glm::translate(c.pos) *
            glm::rotate(glm::radians(c.rot.z), glm::vec3(0.0f, 0.0f, 1.0f)) *
            glm::rotate(glm::radians(c.rot.y), glm::vec3(0.0f, 1.0f, 0.0f)) *
            glm::rotate(glm::radians(c.rot.x), glm::vec3(1.0f, 0.0f, 0.0f)) *
            glm::scale(c.scale);
        glm::mat4 mvp = camera.GetVP() * world;
        glm::mat4 VworldIT =
            glm::transpose(glm::inverse(camera.GetV() * world));
        program.SetUniform("MVP", mvp);
        program.SetUniform("world", world);
        program.SetUniform("VworldIT", VworldIT);
        program.SetUniform("worldIT", glm::transpose(glm::inverse(world)));
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }

    program.Use();
    program.SetUniform("useColor", (int)false);
    program.SetUniform("cam_pos", camera.pos);

    for (auto &c : pyramids) {
        glm::mat4 world =
            glm::translate(c.pos) *
            glm::rotate(glm::radians(c.rot.z), glm::vec3(0.0f, 0.0f, 1.0f)) *
            glm::rotate(glm::radians(c.rot.y), glm::vec3(0.0f, 1.0f, 0.0f)) *
            glm::rotate(glm::radians(c.rot.x), glm::vec3(1.0f, 0.0f, 0.0f)) *
            glm::scale(c.scale);
        glm::mat4 mvp = camera.GetVP() * world;
        glm::mat4 VworldIT =
            glm::transpose(glm::inverse(camera.GetV() * world));
        program.SetUniform("MVP", mvp);
        program.SetUniform("world", world);
        program.SetUniform("VworldIT", VworldIT);
        program.SetUniform("worldIT", glm::transpose(glm::inverse(world)));
        testmesh.Draw();
    }

    sphereProgram.Use();
    sphereProgram.SetUniform("rightVP", cam2.GetVP());
    sphereProgram.SetUniform("VP", cam1.GetVP());
    sphereProgram.SetUniform("invVP", glm::inverse(cam1.GetVP()));
    sphereProgram.SetUniform("V", cam1.GetV());
    sphereProgram.SetUniform("resx", calibSim.resolution_px.x);
    sphereProgram.SetUniform("cam_pos", cam1.pos);
    for (auto &s : spheres) {
        sphereProgram.SetUniform("center", s.pos);
        sphereProgram.SetUniform("radius", s.radius);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
}

void App::ClearMask() {
    GLint fboid = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &fboid);
    if (fboid != (GLint)*fboDispSim)
        return;
    auto mask_clear = glm::ivec2(0, 0);
    glClearTexImage(*maskSim->getCurrent(), 0, GL_RG_INTEGER, GL_INT,
                    &mask_clear);
}

void App::OnKeyDown(SDL_KeyboardEvent &event) {
    switch (event.keysym.sym) {
    case SDLK_a:
        a_down = true;
        break;
    case SDLK_d:
        d_down = true;
        break;
    case SDLK_w:
        w_down = true;
        break;
    case SDLK_s:
        s_down = true;
        break;
    case SDLK_LSHIFT:
        shift_down = true;
        break;
    case SDLK_LCTRL:
        ctrl_down = true;
        break;
    case SDLK_r:
        if (event.keysym.mod &
            (SDL_Keymod::KMOD_CTRL | SDL_Keymod::KMOD_LSHIFT))
            recording_start = SDL_GetTicks() / 1000.0f;
        break;
    case SDLK_F10:
        show_gui = !show_gui;
        break;
    }
}

void App::OnKeyUp(SDL_KeyboardEvent &event) {
    switch (event.keysym.sym) {
    case SDLK_a:
        a_down = false;
        break;
    case SDLK_d:
        d_down = false;
        break;
    case SDLK_w:
        w_down = false;
        break;
    case SDLK_s:
        s_down = false;
        break;
    case SDLK_LSHIFT:
        shift_down = false;
        break;
    case SDLK_LCTRL:
        ctrl_down = false;
        break;
    }
}

void App::OnMouseMove(SDL_MouseMotionEvent &event) {
    if (event.state & SDL_BUTTON_LEFT) {
        controlledCamera->alpha += event.xrel * mouse_sensitivity;
        controlledCamera->beta += event.yrel * mouse_sensitivity;
        controlledCamera->beta =
            glm::clamp(controlledCamera->beta, 0.01f, glm::pi<float>() - 0.01f);
        glm::vec3 spherePos = {
            cosf(controlledCamera->alpha) * sinf(controlledCamera->beta),
            cosf(controlledCamera->beta),
            sinf(controlledCamera->alpha) * sinf(controlledCamera->beta)};
        controlledCamera->at = controlledCamera->pos + spherePos;
    }
}

void App::SwitchMode(bool toloaded) {
    use_simulation = !toloaded;
    glm::ivec2 newres =
        toloaded ? calibLoad->resolution_px : calibSim.resolution_px;

    for (auto &step : intermediateSteps) {
        step->ResizeOwnedTextures(newres);
    }

    firstTransformStep->SetBaseTexture(toloaded ? dispLoad : dispSim);
    pointCloudRenderStep->SetColorsInputTexture(toloaded ? colLoad : col1Sim);
}

void App::Resize(int w, int h) {
    if (h == 0)
        return;

    win_w = w;
    win_h = h;

    // cam1.aspect = ((float)win_w) / win_h;
    // cam2.aspect = ((float)win_w) / win_h;
    freeCam.aspect = ((float)win_w) / win_h;

    // calibSim = CalibrationData(calibSim.baseline_mm,
    // calibSim.resolution_px,
    //                            cam1.cliplPlanes.x, cam1.fovy,
    //                            cam1.aspect);
}

void App::ResizeStereoCameraFbos(glm::ivec2 res) {
    col1Sim->assign(std::make_shared<Texture2D>(res.x, res.y, GL_RGB8));
    col2Sim->assign(std::make_shared<Texture2D>(res.x, res.y, GL_RGB8));
    depth1Sim->assign(std::make_shared<Texture2D>(
        res.x, res.y, GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT));
    depth2Sim->assign(std::make_shared<Texture2D>(
        res.x, res.y, GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT));
    posSim->assign(std::make_shared<Texture2D>(res.x, res.y, GL_RGB32F));
    normSim->assign(std::make_shared<Texture2D>(res.x, res.y, GL_RGBA32F));
    dispSim->assign(std::make_shared<Texture2D>(res.x, res.y, GL_R32F));
    maskSim->assign(
        std::make_shared<Texture2D>(res.x, res.y, GL_RG32I, GL_RG_INTEGER));

    fboDispSim = std::unique_ptr<FrameBufferObject>(new FrameBufferObject(
        {{GL_COLOR_ATTACHMENT0, *col1Sim->getCurrent()},
         {GL_COLOR_ATTACHMENT1, *dispSim->getCurrent()},
         {GL_COLOR_ATTACHMENT2, *posSim->getCurrent()},
         {GL_COLOR_ATTACHMENT3, *normSim->getCurrent()},
         {GL_COLOR_ATTACHMENT4, *maskSim->getCurrent()},
         {GL_DEPTH_ATTACHMENT, *depth1Sim->getCurrent()}}));

    fboColSim = std::unique_ptr<FrameBufferObject>(new FrameBufferObject(
        {{GL_COLOR_ATTACHMENT0, *col2Sim->getCurrent()},
         {GL_DEPTH_ATTACHMENT, *depth2Sim->getCurrent()}}));
}

const std::vector<Vertex> App::CubeVertices = {
    // FRONT
    {{1.0f, -1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
    {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
    {{-1.0f, -1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
    {{-1.0f, -1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
    {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
    {{-1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},

    // RIGHT
    {{1.0f, -1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
    {{1.0f, -1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
    {{1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
    {{1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
    {{1.0f, -1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
    {{1.0f, 1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},

    // BACK
    {{1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f, 0.0f}},
    {{-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f, 0.0f}},
    {{1.0f, 1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f, 0.0f}},
    {{-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f, 0.0f}},
    {{-1.0f, 1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f, 0.0f}},
    {{1.0f, 1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f, 0.0f}},

    // LEFT
    {{-1.0f, -1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 1.0f}},
    {{-1.0f, 1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 1.0f}},
    {{-1.0f, -1.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 1.0f}},
    {{-1.0f, 1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 1.0f}},
    {{-1.0f, 1.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 1.0f}},
    {{-1.0f, -1.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 1.0f}},

    // TOP
    {{1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
    {{1.0f, 1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
    {{-1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
    {{-1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
    {{1.0f, 1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
    {{-1.0f, 1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},

    // BOTTOM
    {{1.0f, -1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f, 1.0f}},
    {{-1.0f, -1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f, 1.0f}},
    {{1.0f, -1.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f, 1.0f}},
    {{-1.0f, -1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f, 1.0f}},
    {{-1.0f, -1.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f, 1.0f}},
    {{1.0f, -1.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f, 1.0f}},
};

void App::RunSingleTest(TestConfig &config, SDLWindow window) {
    auto calibration =
        std::make_unique<CalibrationData>(config.calibration_file);
    auto disparities = std::make_shared<VirtualTexture>("test disp load");
    disparities->assign(std::make_shared<Texture2D>(config.disparity_file));
    auto gtnormals = std::make_shared<VirtualTexture>("test norm load");
    gtnormals->assign(std::make_shared<Texture2D>(config.gtnormal_fila, true));
    auto mask = std::make_shared<VirtualTexture>("test mask load");
    mask->assign(std::make_shared<Texture2D>(config.mask_file));

    std::vector<std::shared_ptr<Renderstep>> steps;

    // transforms GT normal
    auto transformer =
        std::make_shared<Transformer>(calibration->resolution_px);
    transformer->SetBaseTexture(gtnormals);
    transformer->color_transform = glm::mat4(1.0f);
    transformer->color_transform[0][0] = 2.0f;
    transformer->color_transform[1][1] = 2.0f;
    transformer->color_transform[2][2] = -2.0f;
    transformer->color_transform[3][0] = -1.0f;
    transformer->color_transform[3][1] = -1.0f;
    transformer->color_transform[3][2] = 1.0f;
    steps.push_back(transformer);

    auto triangulatestep =
        std::make_shared<Triangulation>(calibration->resolution_px);
    triangulatestep->SetDisparitiesInputTexture(disparities);
    auto &posTri = triangulatestep->GetOutputs()[0];
    triangulatestep->measurePerformance = true;
    steps.push_back(triangulatestep);

    std::shared_ptr<AffineNormalEstimator> affinenormalstep;
    std::shared_ptr<CrossNormalEstimator> crossnormalstep;
    std::shared_ptr<LaplacianFilter> laplacianstep;
    std::shared_ptr<AffineParameterEstimator> affineparamstep;
    std::shared_ptr<PrincipalComponentAnalysisNE> pcastep;

    if (config.method == 0) {
        laplacianstep =
            std::make_shared<LaplacianFilter>(calibration->resolution_px);
        laplacianstep->SetInputTexture(posTri);
        laplacianstep->measurePerformance = true;
        steps.push_back(laplacianstep);
        // if(config.affine_mode == 2 || config.affine_mode == 5)

        affineparamstep = std::make_shared<AffineParameterEstimator>(
            calibration->resolution_px);
        affineparamstep->SetDisparitiesInputTexture(disparities);
        affineparamstep->SetEdgesInputTexture(laplacianstep->GetOutputs()[0]);
        affineparamstep->SetDepthInputTexture(posTri);
        affineparamstep->mode = config.affine_mode;
        affineparamstep->adaptive_dir_count = config.adaptive_dir_count;
        affineparamstep->adaptive_max_step_count =
            config.adaptive_max_step_count;
        affineparamstep->adaptive_depth_thresh_ratio =
            config.adaptive_depth_thresh_ratio;
        affineparamstep->conv_size = config.conv_size;

        affineparamstep->adaptive_simple_thresh = config.adaptive_simple_thresh;
        affineparamstep->adaptive_cumulative_thresh =
            config.adaptive_cumulative_thresh;
        affineparamstep->test_deg_thresh = config.test_deg_thresh;

        if (affineparamstep->mode == 1) {
            affineparamstep->RecomputeAffineLSQConvFilter(
                glm::pi<float>() / 2, 0.1f,
                calibration->resolution_px.x /
                    ((float)calibration->resolution_px.y),
                calibration->resolution_px);
        } else if (affineparamstep->mode >= 2) {
            affineparamstep->RecomputeAdaptiveDirs();
        }
        auto &affineParams = affineparamstep->GetOutputs()[0];
        affineparamstep->measurePerformance = true;

        steps.push_back(affineparamstep);

        affinenormalstep =
            std::make_shared<AffineNormalEstimator>(calibration->resolution_px);
        affinenormalstep->SetAffineParameterInputTexture(affineParams);
        affinenormalstep->SetPositionsInputTexture(posTri);
        affinenormalstep->measurePerformance = true;
        steps.push_back(affinenormalstep);
    } else if (config.method == 1) {
        crossnormalstep =
            std::make_shared<CrossNormalEstimator>(calibration->resolution_px);
        crossnormalstep->SetPositionsInputTexture(posTri);
        crossnormalstep->measurePerformance = true;
        crossnormalstep->symmetric_mode = true;
        steps.push_back(crossnormalstep);
    } else if (config.method == 2) {
        pcastep = std::make_shared<PrincipalComponentAnalysisNE>(
            calibration->resolution_px);
        pcastep->SetPositionTexture(posTri);
        pcastep->measurePerformance = true;
        pcastep->size = config.pca_size;
        pcastep->version = config.pca_version;
        steps.push_back(pcastep);
    }

    auto texturecompare =
        std::make_shared<TextureComparison>(calibration->resolution_px);
    texturecompare->SetComparisonTexture(transformer->GetOutputs()[0], true);
    if (config.method == 0) {
        auto &normalsaffine = affinenormalstep->GetOutputs()[0];
        texturecompare->SetComparisonTexture(normalsaffine, false);
    } else if (config.method == 1) {
        auto &normalscross = crossnormalstep->GetOutputs()[0];
        texturecompare->SetComparisonTexture(normalscross, false);
    } else if (config.method == 2) {
        auto &normalspca = pcastep->GetOutputs()[0];
        texturecompare->SetComparisonTexture(normalspca, false);
    }
    texturecompare->SetMaskTexture(mask);
    texturecompare->mode = TextureComparison::Mode::NORMAL_ANGLE;
    texturecompare->useMask = true;
    texturecompare->useGradient = false;

    auto textureSaver = std::make_shared<SaveTexture>();
    textureSaver->SetTexture(texturecompare->GetOutputs()[0]);
    // textureSaver->SetTexture(normalsaffine);
    // textureSaver->SetTexture(disparities);
    textureSaver->SetMask(mask);
    textureSaver->use_mask = true;
    textureSaver->float_mask = true;

    auto textureview = std::make_shared<TextureViewer>();
    textureview->SetDisplayedTexture(texturecompare->GetOutputs()[0]);
    // textureview->SetDisplayedTexture(mask);
    // textureview->SetDisplayedTexture(disparities);

    // Draw!
    Camera dummyCam;

    float total_time = 0;

    for (int i = 0; i < 100; ++i) {
        glViewport(0, 0, calibration->resolution_px.x,
                   calibration->resolution_px.y);

        for (auto step : steps) {
            step->Render(*calibration, dummyCam);
        }

        // transformer->Render(*calibration, dummyCam);

        // triangulatestep->Render(*calibration, dummyCam);
        // laplacianstep->Render(*calibration, dummyCam);
        // affineparamstep->Render(*calibration, dummyCam);
        // affinenormalstep->Render(*calibration, dummyCam);
        texturecompare->Draw(*calibration, dummyCam);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        textureview->Draw(*calibration, dummyCam);

        SDL_GL_SwapWindow(window);
        glMemoryBarrier(GL_ALL_BARRIER_BITS);

        total_time += triangulatestep->GetLastGpuTimeNs() / 1e6;
        if (config.method == 0) {
            if (config.affine_mode == 2 || config.affine_mode == 5)
                total_time += laplacianstep->GetLastGpuTimeNs() / 1e6;
            total_time += affineparamstep->GetLastGpuTimeNs() / 1e6;
            total_time += affinenormalstep->GetLastGpuTimeNs() / 1e6;
        } else if (config.method == 1) {
            total_time += crossnormalstep->GetLastGpuTimeNs() / 1e6;
        } else if (config.method == 2) {
            total_time += pcastep->GetLastGpuTimeNs() / 1e6;
        }
    }
    float avg_err_rad = textureSaver->GetAvgRed();
    total_time /= 100;
    float avg_err_deg = avg_err_rad / glm::pi<float>() * 180;
    std::ofstream test_out("test_output.txt");
    test_out << std::setprecision(15);
    test_out << avg_err_deg << ' ' << total_time << '\n';
    test_out.close();

    texturecompare->useGradient = true;
    texturecompare->Draw(*calibration, dummyCam);
    SDL_GL_SwapWindow(window);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
    textureSaver->SaveImage("errors_visualized.png");
}