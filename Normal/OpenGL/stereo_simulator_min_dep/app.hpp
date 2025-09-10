#pragma once


#define GLM_ENABLE_EXPERIMENTAL
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform2.hpp>

#include <memory>
#include <random>
#include <vector>

#include <ArrayBuffer.hpp>
#include <FrameBufferObject.hpp>
#include <Mesh.hpp>
#include <ProgramObject.hpp>
#include <ShaderObject.hpp>
#include <Texture2D.hpp>
#include <VertexArrayObject.hpp>

#include "calibration.hpp"
#include "camera.hpp"
#include "renderstep.hpp"
#include "save_texture.hpp"
#include "sliding_statistics.hpp"

#include <imgui.h>
#include "pca.hpp"
#include "testconfig.hpp"

// for cmdline test
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <miniwrapper.hpp>

struct Vertex {
    glm::vec3 p;
    glm::vec3 n;
    glm::vec3 c;
};

class App {
  public:
    App();
    void Update();
    void Render();
    void DrawGUI();

    void Resize(int w, int h);
    void OnKeyDown(SDL_KeyboardEvent &event);
    void OnKeyUp(SDL_KeyboardEvent &event);
    void OnMouseMove(SDL_MouseMotionEvent &event);

    void RunSingleTest(TestConfig& config, SDLWindow window);

  private:
    Camera cam1, cam2, freeCam;
    int win_w, win_h;
    bool a_down = false, w_down = false, d_down = false, s_down = false,
         shift_down = false, ctrl_down = false;
    float move_sensitivity = 5.0f;
    float mouse_sensitivity = 0.01f;
    Camera *controlledCamera;

    float rotationOffset = 0.0f;
    float rotationSpeed = 0.3f;
    float height = -1.0f;
    ArrayBuffer<Vertex> cubeBuffer;
    VertexArrayObject cubeVao;
    void DrawScene(ProgramObject &program, Camera &camera);

    bool use_simulation = true;
    CalibrationData calibSim;
    std::unique_ptr<CalibrationData> calibLoad;
    bool loadResOverride = false;
    glm::vec2 resOverride = {0.5,0.5};

    std::shared_ptr<VirtualTexture> col1Sim;
    std::shared_ptr<VirtualTexture> col2Sim;
    std::shared_ptr<VirtualTexture> depth1Sim;
    std::shared_ptr<VirtualTexture> depth2Sim;
    std::shared_ptr<VirtualTexture> posSim;
    std::shared_ptr<VirtualTexture> normSim;
    std::shared_ptr<VirtualTexture> dispSim;
    std::shared_ptr<VirtualTexture> maskSim;
    std::vector<std::shared_ptr<VirtualTexture>> simulationTextures;

    void ClearMask();

    std::unique_ptr<FrameBufferObject> fboDispSim;
    std::unique_ptr<FrameBufferObject> fboColSim;
    void ResizeStereoCameraFbos(glm::ivec2 res);

    std::shared_ptr<VirtualTexture> colLoad;
    std::shared_ptr<VirtualTexture> dispLoad;
    std::shared_ptr<VirtualTexture> normLoad;
    std::shared_ptr<VirtualTexture> maskLoad;
    std::vector<std::shared_ptr<VirtualTexture>> loadedTextures;

    ProgramObject simDispProgram;
    ProgramObject simColProgram;

    ProgramObject sphereProgram;

    static const std::vector<Vertex> CubeVertices;

    std::vector<std::shared_ptr<Renderstep>> intermediateSteps;
    std::vector<std::shared_ptr<Renderstep>> displaySteps;

    std::shared_ptr<Transformer> firstTransformStep;
    std::shared_ptr<PointCloudVisualizer> pointCloudRenderStep;

    std::shared_ptr<MeasureDifference> measureDifferenceStep;
    std::shared_ptr<NoiseAdder> noisestep;
    std::shared_ptr<AffineParameterEstimator> affinestep;
    std::shared_ptr<PrincipalComponentAnalysisNE> pcarenderstep;
    std::shared_ptr<Transformer> transformstep;
    std::shared_ptr<Transformer> transformstep2;

    void IntermediateStepMenu(std::shared_ptr<Renderstep> step);
    void SwitchTexture(TextureSlot &slot);

    void SwitchMode(bool toloaded);

    Mesh testmesh;
    Mesh sphereMesh;
    Mesh bunnyMesh;

    u_int64_t totalGpuTimeNs;
    SlidingAverage<double> avgGpuTime;
    bool measureOnlyTotalGPU = false;

    void MeasurePerformance();

    std::shared_ptr<SaveTexture> textureSaver;

    // scenes
    const static size_t scene_count = 4;
    const char *scene_names[scene_count] = {"3 Meshes", "Stability test",
                                            "Sphere", "Discontinuity"};
    int selected_scene = 0;

    void DrawScene3Meshes(ProgramObject &program, Camera &camera);
    void DrawSceneStabilityTest(ProgramObject &program, Camera &camera);
    void DrawSceneSphere(ProgramObject &program, Camera &camera);
    void DrawSceneDiscontinuity(ProgramObject &program, Camera &camera);

    using SceneDrawer = void (App::*)(ProgramObject &, Camera &);
    SceneDrawer scene_functions[scene_count] = {
        &App::DrawScene3Meshes, &App::DrawSceneStabilityTest,
        &App::DrawSceneSphere, &App::DrawSceneDiscontinuity};

    struct Transform {
        glm::vec3 pos;
        glm::vec3 rot;
        glm::vec3 scale;
    };

    struct Sphere {
        glm::vec3 pos;
        float radius;
    };

    glm::vec4 background_color = {0.0f, 0.1f, 0.2f, 1.0f};


    float recording_start = 0;
    bool show_gui = true;
    bool two_panel_mode = false;
    bool half_vert_res = false;

    //dejavu-sans.condensed-bold.ttf
    ImFont* overlayfont;
    bool show_animated_value_overlay = true;
};
