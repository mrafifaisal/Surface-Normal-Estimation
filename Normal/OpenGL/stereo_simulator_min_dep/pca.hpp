#pragma once

#include "renderstep.hpp"

class PrincipalComponentAnalysisNE : public Renderstep {
  public:
    PrincipalComponentAnalysisNE(
        glm::ivec2 outputres,
        std::string screen_vertex_shader = "shaders/modular/screen.vert",
        std::string pca_fragment_shader = "shaders/modular/ne_svd_pca.frag");

        virtual ~PrincipalComponentAnalysisNE();
        virtual std::string name() const override;

        virtual void ResizeOwnedTextures(glm::ivec2 newres) override;
        virtual void Draw(CalibrationData& curCalib, Camera& curCam) override;
        virtual void DrawGui() override;
        virtual void ReloadShaders() override;

        void SetPositionTexture(std::shared_ptr<VirtualTexture> tex);

    int size = 3;
    int version = 0;
    private:
    std::string screen_shader_location;
    std::string pca_shader_location;

    std::shared_ptr<VirtualTexture> normalTex;
    std::unique_ptr<FrameBufferObject> normalFbo;

    const char* version_names[2] = {"Modified SVD", "Viktor's"};
};