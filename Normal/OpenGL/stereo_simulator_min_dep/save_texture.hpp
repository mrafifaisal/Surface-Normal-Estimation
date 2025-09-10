#pragma once
#include "renderstep.hpp"
#include <glm/glm.hpp>

class SaveTexture : public Renderstep {
  public:
    SaveTexture(
        glm::ivec2 outputres = {0, 0},
        std::string screen_vertex_shader = "shaders/modular/screen.vert",
        std::string comparison_fragment_shader = "shaders/modular/color.frag");
    virtual ~SaveTexture();
    virtual std::string name() const override;

    virtual void Draw(CalibrationData &curCalib, Camera &curCam) override{};
    virtual void DrawGui() override;
    void SetTexture(std::shared_ptr<VirtualTexture> tex);
    void SetMask(std::shared_ptr<VirtualTexture> new_mask);

    float GetAvgRed();
    void SaveImage(std::string path);

    void SaveImage(std::string path, const std::vector<glm::vec4> &pixels,
                   glm::ivec2 size);
    bool use_mask = true;
    bool float_mask = true;
  private:
    void SaveStats(std::string path, const std::vector<glm::vec4> &pixels,
                   const std::vector<glm::ivec2> &mask, glm::ivec2 size);

    glm::vec4 scale = {1, 1, 1, 1};
    glm::vec4 offset = {0, 0, 0, 0};
    glm::vec4 overwrite = {0, 0, 0, 1};
    glm::bvec4 enable_overwrite = {false, false, false, true};

    bool save_raw = true;
    int sort_idx = 0;
    bool filter_nans = true;
    int border = 0;
};