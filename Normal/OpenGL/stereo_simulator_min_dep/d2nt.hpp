#include "renderstep.hpp"

class GradientFilter : public Renderstep {
  public:
    GradientFilter(
        glm::ivec2 outputres,
        std::string screen_vertex_shader = "shaders/modular/screen.vert",
        std::string gradient_fragment_shader = "shaders/modular/gradient.frag");
    virtual ~GradientFilter();
    virtual std::string name() const override;

    virtual void ResizeOwnedTextures(glm::ivec2 newres) override;
    virtual void Draw(CalibrationData &curCalib, Camera &curCam) override;
    virtual void DrawGui() override;
    void SetInputTexture(std::shared_ptr<VirtualTexture> tex);
    void SetLaplacianTexture(std::shared_ptr<VirtualTexture> tex);

  private:
    std::unique_ptr<FrameBufferObject> fbo;
    std::shared_ptr<VirtualTexture> gradTex;
    bool dag = false;
};

class LaplacianFilter : public Renderstep {
  public:
    LaplacianFilter(
        glm::ivec2 outputres,
        std::string screen_vertex_shader = "shaders/modular/screen.vert",
        std::string gradient_fragment_shader = "shaders/modular/laplacian.frag");
    virtual ~LaplacianFilter();
    virtual std::string name() const override;

    virtual void ResizeOwnedTextures(glm::ivec2 newres) override;
    virtual void Draw(CalibrationData &curCalib, Camera &curCam) override;
    virtual void DrawGui() override;
    void SetInputTexture(std::shared_ptr<VirtualTexture> tex);

    virtual void ReloadShaders() override;

  private:
    std::unique_ptr<FrameBufferObject> fbo;
    std::shared_ptr<VirtualTexture> lapTex;

    std::string frag_loc;
    std::string vert_loc;
};

class DepthToNormalTranslator : public Renderstep {
  public:
    DepthToNormalTranslator(
        glm::ivec2 outputres,
        std::string screen_vertex_shader = "shaders/modular/screen.vert",
        std::string d2nt_fragment_shader = "shaders/modular/ne_d2nt.frag");
    virtual ~DepthToNormalTranslator();
    virtual std::string name() const override;

    virtual void ResizeOwnedTextures(glm::ivec2 newres) override;
    virtual void Draw(CalibrationData &curCalib, Camera &curCam) override;
    virtual void DrawGui() override;
    void SetDepthTexture(std::shared_ptr<VirtualTexture> tex);
    void SetGradientTexture(std::shared_ptr<VirtualTexture> tex);

  private:
    std::unique_ptr<FrameBufferObject> fbo;
    std::shared_ptr<VirtualTexture> normTex;
};

class MRFRefiner : public Renderstep {
  public:
    MRFRefiner(
        glm::ivec2 outputres,
        std::string screen_vertex_shader = "shaders/modular/screen.vert",
        std::string mrf_fragment_shader = "shaders/modular/mrf_refine.frag");
    virtual ~MRFRefiner();
    virtual std::string name() const override;

    virtual void ResizeOwnedTextures(glm::ivec2 newres) override;
    virtual void Draw(CalibrationData &curCalib, Camera &curCam) override;
    virtual void DrawGui() override;
    void SetNormalTexture(std::shared_ptr<VirtualTexture> tex);
    void SetLaplacianTexture(std::shared_ptr<VirtualTexture> tex);

  private:
    std::unique_ptr<FrameBufferObject> fbo;
    std::shared_ptr<VirtualTexture> normTex;

    bool passthrough = false;
};