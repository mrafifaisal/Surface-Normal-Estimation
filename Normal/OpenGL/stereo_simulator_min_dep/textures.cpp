#include "textures.hpp"
#include <stdexcept>

VirtualTexture::VirtualTexture(std::string name,
                               std::shared_ptr<Texture2D> texture) {
    this->displayName = name;
    this->texture = texture;
}

std::shared_ptr<Texture2D> VirtualTexture::getCurrent() const {
    if (this->texture == nullptr) {
        throw std::runtime_error("Virtual texture not set: " + displayName);
    }
    return this->texture;
}

void VirtualTexture::assign(std::shared_ptr<Texture2D> texture) {
    this->texture = texture;
}

TextureSlot::TextureSlot(std::string name) : name(name), texture(nullptr) {
    name_with_content = name + "(<none>)";
}
void TextureSlot::Set(std::shared_ptr<VirtualTexture> texture) {
    this->texture = texture;
    std::string texture_name =
        texture != nullptr ? texture->displayName : "<none>";
    this->name_with_content = name + " (" + texture_name + ")";
}
std::shared_ptr<VirtualTexture> TextureSlot::Get() const { return texture; }

std::shared_ptr<Texture2D> TextureSlot::GetTex() const {
    if (texture == nullptr)
        throw std::runtime_error("input slot not set: " + name);
    return texture->getCurrent();
}

bool TextureSlot::IsSet() const { return texture != nullptr; }