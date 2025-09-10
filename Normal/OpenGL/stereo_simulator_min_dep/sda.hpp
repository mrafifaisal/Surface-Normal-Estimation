#pragma once

#include "renderstep.hpp"

class SpatialDiscontinuityAwareSNE : public Renderstep {
  public:
    SpatialDiscontinuityAwareSNE(
        glm::ivec2 outputres,
        std::string screen_vertex_shader = "shaders/modular/screen.vert",
        std::string sda_init_fragment_shader =
            "shaders/modular/ne_sda_init.frag",
        std::string sda_step_fragment_shader =
            "shaders/modular/ne_sda_step.frag",
        std::string cp2tv_fragment_shader = "shaders/modular/ne_cp2tv.frag");
    virtual ~SpatialDiscontinuityAwareSNE();
    virtual std::string name() const override;

    virtual void ResizeOwnedTextures(glm::ivec2 newres) override;
    virtual void Draw(CalibrationData &curCalib, Camera &curCam) override;
    virtual void DrawGui() override;
    virtual void ReloadShaders() override;

    void SetDepthTexture(std::shared_ptr<VirtualTexture> tex);
    void SetLaplacianTexture(std::shared_ptr<VirtualTexture> tex);

  private:
    ProgramObject stepShader;
    ProgramObject cp2tvShader;
    std::unique_ptr<FrameBufferObject> fbo1;
    std::unique_ptr<FrameBufferObject> fbo2;
    std::shared_ptr<VirtualTexture> gradAndEnergyTex1;
    std::shared_ptr<VirtualTexture> minindexTex1;
    std::shared_ptr<VirtualTexture> gradAndEnergyTex2;
    std::shared_ptr<VirtualTexture> minindexTex2;

    std::shared_ptr<VirtualTexture> normalTex;
    std::unique_ptr<FrameBufferObject> normalFbo;

    std::string screen_shader_location;
    std::string sda_init_shader_location;
    std::string sda_step_shader_location;
    std::string cp2tv_shader_location;

    std::shared_ptr<VirtualTexture> final_gradAndEnergy;
    std::shared_ptr<VirtualTexture> final_minindex;

    int sda_iters = 5;

    bool test_gradient = false;

    // std::shared_ptr<VirtualTexture> energyTex;
};