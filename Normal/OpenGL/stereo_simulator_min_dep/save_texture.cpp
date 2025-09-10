#include "save_texture.hpp"
#include "date.h"
#include <Magick++.h>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <imgui.h>
#include <nfd.h>
#include <string>

SaveTexture::SaveTexture(glm::ivec2 a, std::string b, std::string c)
    : Renderstep({GL_NONE}, false) {
    inputTextures.emplace_back("texture");
    inputTextures.emplace_back("mask");
}

SaveTexture::~SaveTexture() {}

std::string SaveTexture::name() const { return "Save Texture"; }

void SaveTexture::DrawGui() {

    ImGui::SeparatorText("Global");
    ImGui::InputInt("ignored border width", &border);
    ImGui::Checkbox("Mask", &use_mask);
    ImGui::Checkbox("Float mask",&float_mask);

    ImGui::PushID("image file");
    ImGui::SeparatorText("Image export");

    ImGui::InputFloat4("color scale", &scale[0]);
    ImGui::InputFloat4("color offset", &offset[0]);
    ImGui::InputFloat4("color overwrite", &overwrite[0]);
    ImGui::Text("Overwrite: ");
    ImGui::SameLine();
    ImGui::Checkbox("X", &enable_overwrite[0]);
    ImGui::SameLine();
    ImGui::Spacing();
    ImGui::SameLine();
    ImGui::Checkbox("Y", &enable_overwrite[1]);
    ImGui::SameLine();
    ImGui::Spacing();
    ImGui::SameLine();
    ImGui::Checkbox("Z", &enable_overwrite[2]);
    ImGui::SameLine();
    ImGui::Spacing();
    ImGui::SameLine();
    ImGui::Checkbox("A", &enable_overwrite[3]);

    static char selected_file[1000] = "output.txt";
    ImGui::InputText("file: ", selected_file, 1000);
    // ImGui::Text(selected_file.c_str());
    ImGui::SameLine();
    if (ImGui::Button("select")) {
        nfdchar_t *outPath;
        nfdresult_t result =
            NFD_SaveDialog(&outPath, nullptr, 0, nullptr, nullptr);
        if (result == NFD_OKAY) {
            std::string tmp = outPath;
            strcpy(selected_file, tmp.c_str());
            NFD_FreePath(outPath);
        }
    }
    ImGui::PopID();

    ImGui::PushID("raw file");
    ImGui::SeparatorText("Statistics + raw export");
    ImGui::Checkbox("save statistics + raw data as well", &save_raw);
    ImGui::Checkbox("filter NaNs", &filter_nans);
    if (ImGui::InputInt("sort coord", &sort_idx)) {
        sort_idx = glm::clamp(sort_idx, 0, 3);
    }
    // static std::string raw_file = "output.txt";
    static char raw_file[1000] = "output.txt";
    ImGui::InputText("file: ", raw_file, 1000);
    ImGui::SameLine();
    if (ImGui::Button("select")) {
        nfdchar_t *outPath;
        nfdresult_t result =
            NFD_SaveDialog(&outPath, nullptr, 0, nullptr, nullptr);
        if (result == NFD_OKAY) {
            std::string tmp = outPath;
            strcpy(raw_file, tmp.c_str());
            NFD_FreePath(outPath);
        }
    }
    ImGui::PopID();

    if (ImGui::Button("Save")) {
        int w, h;
        GLuint id = *inputTextures[0].GetTex();
        glGetTextureLevelParameteriv(id, 0, GL_TEXTURE_WIDTH, &w);
        glGetTextureLevelParameteriv(id, 0, GL_TEXTURE_HEIGHT, &h);

        std::vector<glm::vec4> data(w * h);

        glGetTextureImage(id, 0, GL_RGBA, GL_FLOAT,
                          w * h * sizeof(Magick::Quantum) * 4, data.data());

        GLuint mask_id = *inputTextures[1].GetTex();
        std::vector<glm::ivec2> mask(w*h);
        if(float_mask)
            glGetTextureImage(mask_id, 0, GL_RG, GL_INT,
                          w * h * sizeof(glm::ivec2), mask.data());
        else 
            if(float_mask)
                glGetTextureImage(mask_id, 0, GL_RG_INTEGER, GL_INT,
                          w * h * sizeof(glm::ivec2), mask.data());

        SaveImage(selected_file, data, {w, h});

        if (save_raw)
            SaveStats(raw_file, data, mask, {w, h});
    }
}

float SaveTexture::GetAvgRed() {
    int w, h;
    GLuint id = *inputTextures[0].GetTex();
    glGetTextureLevelParameteriv(id, 0, GL_TEXTURE_WIDTH, &w);
    glGetTextureLevelParameteriv(id, 0, GL_TEXTURE_HEIGHT, &h);

    std::vector<glm::vec4> data(w * h);

    glGetTextureImage(id, 0, GL_RGBA, GL_FLOAT,
                        w * h * sizeof(Magick::Quantum) * 4, data.data());

    GLuint mask_id = *inputTextures[1].GetTex();
    std::vector<glm::ivec2> mask(w*h);
    if(float_mask)
        glGetTextureImage(mask_id, 0, GL_RG, GL_INT,
                        w * h * sizeof(glm::ivec2), mask.data());
    else 
        if(float_mask)
            glGetTextureImage(mask_id, 0, GL_RG_INTEGER, GL_INT,
                        w * h * sizeof(glm::ivec2), mask.data());

    std::vector<glm::vec4> pixels;
    auto& pixels_input = data;
    glm::ivec2 res = {w,h};
    pixels.reserve(pixels_input.size());

    for (int i = border; i < res.x - border; ++i) {
        for (int j = border; j < res.y - border; ++j) {
            size_t idx = j * res.x + i;
            if(use_mask && mask[idx].x == 0)
                continue;
            auto &px = pixels_input[idx];
            if (!(filter_nans && std::isnan(px.x + px.y + px.z + px.w)))
                pixels.push_back(px);
        }
    }

    glm::vec4 mean = glm::vec4(0);
    std::vector<float> sort_coords;
    sort_coords.reserve(pixels.size());
    for (auto &px : pixels) {
        mean += px;
        sort_coords.push_back(px[sort_idx]);
    }
    mean /= pixels.size();

    std::nth_element(sort_coords.begin(),
                     sort_coords.begin() + sort_coords.size() / 2,
                     sort_coords.end());
    float median = sort_coords[sort_coords.size() / 2];

    if (sort_coords.size() % 2 == 0) {
        std::nth_element(sort_coords.begin(),
                         sort_coords.begin() + sort_coords.size() / 2 - 1,
                         sort_coords.end());
        median += sort_coords[sort_coords.size() / 2 - 1];
        median /= 2.0f;
    }

    return mean.x;
}

void SaveTexture::SetTexture(std::shared_ptr<VirtualTexture> tex) {
    inputTextures[0].Set(tex);
}

void SaveTexture::SetMask(std::shared_ptr<VirtualTexture> new_mask) {
    inputTextures[1].Set(new_mask);
}

void SaveTexture::SaveImage(std::string path) {
    int w, h;
    GLuint id = *inputTextures[0].GetTex();
    glGetTextureLevelParameteriv(id, 0, GL_TEXTURE_WIDTH, &w);
    glGetTextureLevelParameteriv(id, 0, GL_TEXTURE_HEIGHT, &h);

    std::vector<glm::vec4> data(w * h);

    glGetTextureImage(id, 0, GL_RGBA, GL_FLOAT,
                        w * h * sizeof(Magick::Quantum) * 4, data.data());

    SaveImage(path, data, {w,h});
}

void SaveTexture::SaveImage(std::string path,
                            const std::vector<glm::vec4> &pixels,
                            glm::ivec2 size) {
    int w = size.x;
    int h = size.y;

    using namespace Magick;
    Magick::Image img(Magick::Geometry(w - 2 * border, h - 2 * border),
                      Magick::Color("white"));
    img.modifyImage();
    img.type(Magick::TrueColorAlphaType);
    Magick::Quantum *pixel_cache =
        img.getPixels(0, 0, w - 2 * border, h - 2 * border);

    for (int i = border; i < w - border; ++i) {
        for (int j = border; j < h - border; ++j) {
            size_t idx = j * w + i;
            glm::dvec4 c = pixels[idx];

            c *= scale;
            c += offset;
            for (int k = 0; k < 4; ++k)
                if (enable_overwrite[k])
                    c[k] = overwrite[k];
            c *= QuantumRange;

            idx = (j - border) * (w - 2 * border) + (i - border);
            pixel_cache[idx * 4 + 0] = c.x;
            pixel_cache[idx * 4 + 1] = c.y;
            pixel_cache[idx * 4 + 2] = c.z;
            pixel_cache[idx * 4 + 3] = c.a;
        }
    }

    img.syncPixels();
    img.flip();
    img.write(path);

    //     Image image("cow.png");
    // // Ensure that there are no other references to this image.
    // image.modifyImage();
    // // Set the image type to TrueColor DirectClass representation.
    // image.type(TrueColorType);
    // // Request pixel region with size 60x40, and top origin at 20x30
    // ssize_t columns = 60;
    // Quantum *pixel_cache = image.getPixels(20,30,columns,40);
    // // Set pixel at column 5, and row 10 in the pixel cache to red.
    // ssize_t column = 5;
    // ssize_t row = 10;
    // Quantum *pixel = pixel_cache+row*columns+column;
    // *pixel = Color("red");
    // // Save changes to underlying image .
    // image.syncPixels();
    //   // Save updated image to file.
    // image.write("horse.png");
}

void SaveTexture::SaveStats(std::string path,
                            const std::vector<glm::vec4> &pixels_input,
                            const std::vector<glm::ivec2> &mask,
                            glm::ivec2 res) {
    std::vector<glm::vec4> pixels;
    pixels.reserve(pixels_input.size());

    for (int i = border; i < res.x - border; ++i) {
        for (int j = border; j < res.y - border; ++j) {
            size_t idx = j * res.x + i;
            if(use_mask && mask[idx].x == 0)
                continue;
            auto &px = pixels_input[idx];
            if (!(filter_nans && std::isnan(px.x + px.y + px.z + px.w)))
                pixels.push_back(px);
        }
    }

    glm::vec4 mean = glm::vec4(0);
    std::vector<float> sort_coords;
    sort_coords.reserve(pixels.size());
    for (auto &px : pixels) {
        mean += px;
        sort_coords.push_back(px[sort_idx]);
    }
    mean /= pixels.size();

    std::nth_element(sort_coords.begin(),
                     sort_coords.begin() + sort_coords.size() / 2,
                     sort_coords.end());
    float median = sort_coords[sort_coords.size() / 2];

    if (sort_coords.size() % 2 == 0) {
        std::nth_element(sort_coords.begin(),
                         sort_coords.begin() + sort_coords.size() / 2 - 1,
                         sort_coords.end());
        median += sort_coords[sort_coords.size() / 2 - 1];
        median /= 2.0f;
    }

    std::ofstream file;
    file.open(path, std::ios_base::openmode::_S_app);
    std::string renderer = (char *)glGetString(GL_RENDERER);
    file << date::format("%F %T", std::chrono::system_clock::now());
    file << ";" << renderer << ";" << res.x << "x" << res.y << "\n";
    file << "Mean;Median IDX (0-3);Median"
         << "\n";
    file << mean.x << " " << mean.y << " " << mean.z << " " << mean.w;
    file << ";" << sort_idx << ";" << median << "\n";

    file << "Values:\n";
    file << "R;G;B;A\n";
    for (auto &px : pixels) {
        file << px.x << ";" << px.y << ";" << px.z << ";" << px.a << "\n";
    }

    file.close();
}
