#pragma once

#include "renderstep.hpp"

class CannyEdgeDetector : public Renderstep {
  public:
    CannyEdgeDetector(
        glm::ivec2 outputres,
        std::string screen_vertex_shader = "shaders/modular/screen.vert",
        std::string canny_prepare_fragment_shader = "shaders/modular/canny_prepare.frag",
        std::string canny_fragment_shader = "shaders/modular/canny.frag");

        virtual ~CannyEdgeDetector();
        virtual std::string name() const override;

        virtual void ResizeOwnedTextures(glm::ivec2 newres) override;
        virtual void Draw(CalibrationData& curCalib, Camera& curCam) override;
        virtual void DrawGui() override;
        virtual void ReloadShaders() override;

        void SetTexture(std::shared_ptr<VirtualTexture> tex);

    private:
    std::string screen_shader_location;
    std::string canny_shader_location;
    std::string prepare_shader_location;

    std::shared_ptr<VirtualTexture> preparedTex;
    std::unique_ptr<FrameBufferObject> prepareFbo;
    ProgramObject prepareShader;

    std::shared_ptr<VirtualTexture> edgeTex;
    std::unique_ptr<FrameBufferObject> edgeFbo;

    float weakThreshold = 1.0f/3;
    float strongThreshold = 1.0f;
};