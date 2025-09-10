#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <FrameBufferObject.hpp>
#include <ProgramObject.hpp>
#include <Texture2D.hpp>
#include <random>
#include <vector>

#include "calibration.hpp"
#include "camera.hpp"
#include "textures.hpp"

class Renderstep {
  public:
    Renderstep(ProgramObject shader, bool shaderReload = false);
    void Render(CalibrationData &curCalib, Camera &curCam);
    virtual void DrawGui();
    virtual void ResizeOwnedTextures(glm::ivec2 newres);
    virtual std::string name() const = 0;

    virtual void ReloadShaders();

    std::vector<TextureSlot> &GetInputs();
    const std::vector<std::shared_ptr<VirtualTexture>> &GetOutputs() const;

    virtual ~Renderstep();
    bool enabled = true;
    bool measurePerformance = false;

    u_int64_t GetLastGpuTimeNs() const;
    const bool shaderReloadable;

    virtual bool RenderToOtherPanel() const { return false; }

  protected:
    virtual void Draw(CalibrationData &curCalib, Camera &curCam) = 0;
    ProgramObject shader;
    std::vector<TextureSlot> inputTextures;
    std::vector<std::shared_ptr<VirtualTexture>> outputTextures;
    /// @brief Last measured execution time of the GPU drawcall in nanoseconds
    u_int64_t lastGpuTimeNs;
};

class PointCloudVisualizer : public Renderstep {
  public:
    PointCloudVisualizer(std::string pointcloud_shader_path,
                         std::string color_shader_path);
    PointCloudVisualizer();
    virtual ~PointCloudVisualizer();
    virtual std::string name() const override;

    virtual void Draw(CalibrationData &curCalib, Camera &curCam) override;
    virtual void DrawGui() override;
    void SetPositionsInputTexture(std::shared_ptr<VirtualTexture> tex);
    void SetColorsInputTexture(std::shared_ptr<VirtualTexture> tex);

  private:
    glm::ivec2 nth = {1, 1};
    float pointSize = 3.0f;
};

class Triangulation : public Renderstep {
  public:
    Triangulation(
        glm::ivec2 outputres,
        std::string screen_vertex_shader = "shaders/modular/screen.vert",
        std::string triangulation_fragment_shader =
            "shaders/modular/triangulate.frag");
    virtual std::string name() const override;

    virtual ~Triangulation() override;

    virtual void ResizeOwnedTextures(glm::ivec2 newres) override;
    virtual void Draw(CalibrationData &curCalib, Camera &curCam) override;
    virtual void DrawGui() override;
    virtual void ReloadShaders() override;
    void SetDisparitiesInputTexture(std::shared_ptr<VirtualTexture> tex);

    std::unique_ptr<FrameBufferObject> fbo;
    std::shared_ptr<VirtualTexture> posTex;

  private:
    float scale = 1.0f;

    std::string vertex_shader;
    std::string fragment_shader;
};

class NormalVisualizer : public Renderstep {
  public:
    NormalVisualizer(
        std::string vertex_shader = "shaders/modular/normals.vert",
        std::string geometry_shader = "shaders/modular/normals.geom",
        std::string fragment_shader = "shaders/modular/color.frag");
    virtual ~NormalVisualizer();
    virtual std::string name() const override;

    virtual void Draw(CalibrationData &curCalib, Camera &curCam) override;
    virtual void DrawGui() override;
    void SetPositionsInputTexture(std::shared_ptr<VirtualTexture> tex);
    void SetNormalsInputTexture(std::shared_ptr<VirtualTexture> tex);
    void SetMasksInputTexture(std::shared_ptr<VirtualTexture> tex);

  private:
    float length = 0.1f;
    glm::vec3 color = glm::vec3(1.0f, 0.0f, 1.0f);
    glm::ivec2 nth = {3, 3};
    float lineWidth = 1.0f;
    bool antiAliasing = true;
    bool bothSides = false;
    bool smartInvert = true;
    bool distance_independent_length = true;
    bool useMask = true;
};

class CrossNormalEstimator : public Renderstep {
  public:
    CrossNormalEstimator(
        glm::ivec2 outputres,
        std::string screen_vertex_shader = "shaders/modular/screen.vert",
        std::string cne_fragment_shader = "shaders/modular/ne_cross.frag");
    virtual ~CrossNormalEstimator();
    virtual std::string name() const override;

    virtual void ResizeOwnedTextures(glm::ivec2 newres) override;
    virtual void Draw(CalibrationData &curCalib, Camera &curCam) override;
    virtual void DrawGui() override;
    void SetPositionsInputTexture(std::shared_ptr<VirtualTexture> tex);

    std::unique_ptr<FrameBufferObject> fbo;
    std::shared_ptr<VirtualTexture> normTex;

    bool symmetric_mode = false;
  private:
};

class AffineNormalEstimator : public Renderstep {
  public:
    AffineNormalEstimator(
        glm::ivec2 outputres,
        std::string screen_vertex_shader = "shaders/modular/screen.vert",
        std::string cne_fragment_shader = "shaders/modular/ne_affine.frag");
    virtual ~AffineNormalEstimator();
    virtual std::string name() const override;

    virtual void ResizeOwnedTextures(glm::ivec2 newres) override;
    virtual void Draw(CalibrationData &curCalib, Camera &curCam) override;
    virtual void DrawGui() override;
    void SetAffineParameterInputTexture(std::shared_ptr<VirtualTexture> tex);
    void SetPositionsInputTexture(std::shared_ptr<VirtualTexture> tex);

    std::unique_ptr<FrameBufferObject> fbo;
    std::shared_ptr<VirtualTexture> normTex;

  private:
    bool symmetric_mode = false;
};

class AffineParameterEstimator : public Renderstep {
  public:
    AffineParameterEstimator(
        glm::ivec2 outputres,
        std::string screen_vertex_shader = "shaders/modular/screen.vert",
        std::string affine_fragment_shader = "shaders/modular/affine.frag");
    virtual ~AffineParameterEstimator();
    virtual std::string name() const override;

    virtual void ResizeOwnedTextures(glm::ivec2 newres) override;
    virtual void Draw(CalibrationData &curCalib, Camera &curCam) override;
    virtual void DrawGui() override;
    virtual void ReloadShaders() override;
    void SetDisparitiesInputTexture(std::shared_ptr<VirtualTexture> tex);
    void SetEdgesInputTexture(std::shared_ptr<VirtualTexture> tex);
    void SetDepthInputTexture(std::shared_ptr<VirtualTexture> tex);

    std::unique_ptr<FrameBufferObject> fbo;
    std::shared_ptr<VirtualTexture> affTex;

    int conv_size = 3;
    bool kernels_invalid = false;
    float adaptive_depth_thresh_ratio = 0.1f;
    float adaptive_simple_thresh = 0.5f;
    int adaptive_dir_count = 8;
    int adaptive_max_step_count = 5;
    void RecomputeAffineLSQConvFilter(float fovy, float f, float aspect,
                                      glm::vec2 res);
    void RecomputeAdaptiveDirs();

  // When setting also recompute kernels or set kernels_invalid
    int mode = 0; // 0 normal, 1 lsq, 2 adaptive lsq
    float adaptive_cumulative_thresh = 1.0f;
    float test_deg_thresh = 10;
  private:

    std::vector<float> conv1;
    std::vector<float> conv2;
    std::vector<glm::vec2> v;

    std::vector<glm::vec2> adaptive_dirs;

    std::string screen_shader;
    std::string affine_shader;

};

class TextureComparison : public Renderstep {
  public:
    TextureComparison(
        glm::ivec2 outputres,
        std::string screen_vertex_shader = "shaders/modular/screen.vert",
        std::string comparison_fragment_shader =
            "shaders/modular/comparison.frag");
    virtual ~TextureComparison();
    virtual std::string name() const override;

    virtual void ResizeOwnedTextures(glm::ivec2 newres) override;
    virtual void Draw(CalibrationData &curCalib, Camera &curCam) override;
    virtual void DrawGui() override;
    void SetComparisonTexture(std::shared_ptr<VirtualTexture> tex,
                              bool reference = true);
    void SetMaskTexture(std::shared_ptr<VirtualTexture> tex);

    virtual void ReloadShaders() override;

    std::unique_ptr<FrameBufferObject> fbo;
    std::shared_ptr<VirtualTexture> diffTex;

    enum Mode { DIFFERENCE = 0, NORMAL_ANGLE = 1 };
    Mode mode = NORMAL_ANGLE;
    bool useMask = true;
    bool useGradient = true;
  private:
    const char *option_names[4] = {"Difference", "Normal angle", "Manhattan",
                                   "Euclidean"};


    std::string vert_shader;
    std::string frag_shader;

    void ExportRawComparison(std::string path);
};

class MeasureDifference : public Renderstep {
  public:
    MeasureDifference(
        glm::ivec2 resolution,
        std::string compute_shader = "shaders/modular/errorstats.comp");
    virtual ~MeasureDifference();
    virtual std::string name() const override;

    virtual void Draw(CalibrationData &curCalib, Camera &curCam) override;
    virtual void DrawGui() override;
    virtual void ResizeOwnedTextures(glm::ivec2 newres) override;
    void SetComparisonTexture(std::shared_ptr<VirtualTexture> tex,
                              bool reference = true);
    void SetMaskTexture(std::shared_ptr<VirtualTexture> tex);

    GLuint bufferID = GL_NONE;

  private:
    enum Metrics { EUCLIDEAN = 0, NORMAL_ANGLE = 1 };
    Metrics metric = EUCLIDEAN;
    const char *option_names[2] = {"Euclidean distance", "Normal angle"};

    double lastMeasurement;
    double threshold = -1;
    bool useThreshold = false;
    int margin = 1;
    bool useMask = false;
};

class TextureViewer : public Renderstep {
  public:
    TextureViewer(
        std::string screen_vertex_shader = "shaders/modular/screen.vert",
        std::string view_fragment_shader = "shaders/modular/textureview.frag");
    virtual ~TextureViewer();
    virtual std::string name() const override;

    virtual void Draw(CalibrationData &curCalib, Camera &curCam) override;
    void SetDisplayedTexture(std::shared_ptr<VirtualTexture> tex);
    virtual bool RenderToOtherPanel() const override {return true;}
};

class NoiseAdder : public Renderstep {
  public:
    NoiseAdder(
        glm::ivec2 outputres,
        std::string screen_vertex_shader = "shaders/modular/screen.vert",
        std::string noise_fragment_shader = "shaders/modular/noise.frag");
    virtual ~NoiseAdder();
    virtual std::string name() const override;

    virtual void ResizeOwnedTextures(glm::ivec2 newres) override;
    virtual void Draw(CalibrationData &curCalib, Camera &curCam) override;
    virtual void DrawGui() override;
    void SetBaseTexture(std::shared_ptr<VirtualTexture> tex);

    float stddeviation = 1.0f;
    bool need_regen = false;

  private:
    bool realtime = false;
    bool passthrough = true;

    std::unique_ptr<FrameBufferObject> fbo;
    std::shared_ptr<VirtualTexture> noiseTex;
    std::shared_ptr<VirtualTexture> noisyTex;

    std::default_random_engine rng;

    std::shared_ptr<Texture2D>
    GenerateNoiseTexture(int w, int h, float noise_standard_deviation);
};

class Transformer : public Renderstep {
  public:
    Transformer(
        glm::ivec2 outputres,
        std::string screen_vertex_shader = "shaders/modular/screen.vert",
        std::string transform_fragment_shader =
            "shaders/modular/transform.frag");
    virtual ~Transformer();
    virtual std::string name() const override;

    virtual void ResizeOwnedTextures(glm::ivec2 newres) override;
    virtual void Draw(CalibrationData &curCalib, Camera &curCam) override;
    virtual void DrawGui() override;
    void SetBaseTexture(std::shared_ptr<VirtualTexture> tex);

    bool flipX = false;
    bool flipY = false;
    glm::mat4 color_transform = glm::mat4(1.0f);
    float multiply = 1.0f;
    glm::vec4 color_shift = glm::vec4(0.0f);
    bool normalize_out3 = false;


  private:
    std::unique_ptr<FrameBufferObject> fbo;
    std::shared_ptr<VirtualTexture> transformedTex;
};