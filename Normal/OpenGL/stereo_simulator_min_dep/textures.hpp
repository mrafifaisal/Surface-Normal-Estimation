#pragma once

#include <Texture2D.hpp>
#include <string>
#include <memory>

class VirtualTexture {
  public:
    VirtualTexture(std::string name, std::shared_ptr<Texture2D> texture = nullptr);
    std::shared_ptr<Texture2D> getCurrent() const;
    void assign(std::shared_ptr<Texture2D> texture);

    std::string displayName;
  private:
    std::shared_ptr<Texture2D> texture;
};

class TextureSlot {
  public:
    TextureSlot(std::string name);
    void Set(std::shared_ptr<VirtualTexture> texture);
    std::shared_ptr<VirtualTexture> Get() const;
    std::shared_ptr<Texture2D> GetTex() const;
    std::string name;
    std::string name_with_content;
    bool IsSet() const;
  protected:
    std::shared_ptr<VirtualTexture> texture;
};