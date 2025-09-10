#include "Texture2D.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <exception>
#include <iostream>
#include <vector>

#include <Magick++.h>

// #define cimg_use_png
// #define cimg_use_jpeg
// #define cimg_use_tiff
// #include "CImg.h"

// using namespace cimg_library;

Texture2D::Texture2D(size_t width, size_t height, GLenum format,
                     GLenum srcFormat, GLenum type) {
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, srcFormat,
                 type, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);
    internal_format = format;
    std::cerr << "created texture " << id << "\n";
}

inline int transform_image(int pitch, int height, void *image_pixels);
Texture2D::Texture2D(std::string path, bool normalized) {
    Magick::Image img(path);
    std::cout << "Loaded image: " << img.format() << "\n";
    std::cout << "depth: " << img.depth() << "\n";
    // std::cout << "channels: " << img.channels() << "\n";

    

    int channels = img.channels();
    int depth = img.depth();
    MagickCore::StorageType type;
    GLuint glGpuType;
    GLuint glCpuOrder;
    GLuint glCpuType;
    size_t unitSize;
    std::string map;
    if (depth == 8) {
        type = MagickCore::StorageType::CharPixel;
        glCpuType = GL_UNSIGNED_BYTE;
        unitSize = sizeof(char);
        switch (channels) {
        case 1:
            glGpuType = normalized ? GL_R8_SNORM : GL_R8;
            glCpuOrder = GL_RED;
            map = "R";
            break;
        case 2:
            glGpuType = normalized ? GL_RG8_SNORM : GL_RG8;
            glCpuOrder = GL_RG;
            map = "RG";
            break;
        case 3:
            glGpuType = normalized ? GL_RGB8_SNORM : GL_RGB8;
            glCpuOrder = GL_RGB;
            map = "RGB";
            break;
        case 4:
            glGpuType = normalized ? GL_RGBA8_SNORM : GL_RGBA8;
            glCpuOrder = GL_RGBA;
            map = "RGBA";
            break;
        default:
            throw std::runtime_error(
                "Loading of images with >4 channels is not supported.");
        }
    } else if (depth == 16) {
        type = MagickCore::StorageType::ShortPixel;
        glCpuType = GL_UNSIGNED_SHORT;
        unitSize = sizeof(unsigned short int);
        switch (channels) {
        case 1:
            glGpuType = normalized ? GL_R16_SNORM : GL_R16;
            glCpuOrder = GL_RED;
            map = "R";
            break;
        case 2:
            glGpuType = normalized ? GL_RG16_SNORM : GL_RG16;
            glCpuOrder = GL_RG;
            map = "RG";
            break;
        case 3:
            glGpuType = normalized ? GL_RGB16_SNORM : GL_RGB16;
            glCpuOrder = GL_RGB;
            map = "RGB";
            break;
        case 4:
            glGpuType = normalized ? GL_RGBA16_SNORM : GL_RGBA16;
            glCpuOrder = GL_RGBA;
            map = "RGBA";
            break;
        default:
            throw std::runtime_error(
                "Loading of images with >4 channels is not supported.");
        }
    } else if (depth == 32) {
        type = MagickCore::StorageType::FloatPixel;
        glCpuType = GL_FLOAT;
        unitSize = sizeof(float);
        switch (channels) {
        case 1:
            glGpuType = GL_R32F;
            glCpuOrder = GL_RED;
            map = "R";
            break;
        case 2:
            glGpuType = GL_RG32F;
            glCpuOrder = GL_RG;
            map = "RG";
            break;
        case 3:
            glGpuType = GL_RGB32F;
            glCpuOrder = GL_RGB;
            map = "RGB";
            break;
        case 4:
            glGpuType = GL_RGBA32F;
            glCpuOrder = GL_RGBA;
            map = "RGBA";
            break;
        default:
            throw std::runtime_error(
                "Loading of images with >4 channels is not supported.");
        }
    }

    void *mem =
        malloc(unitSize * img.size().width() * img.size().height() * channels);
    std::cout << "allocated: "
              << unitSize * img.size().width() * img.size().height() * channels
              << " bytes\n";
    img.flip();
    std::cout << "flipped image\n";
    img.write((size_t)0, (size_t)0, img.size().width(), img.size().height(),
              map, type, mem);
    std::cout << "wrote image to memory\n";

    // SDL_Surface *surf = IMG_Load(path.c_str());
    // if (surf == nullptr) {
    //     std::cerr << "Failed to load texture: " << path << "\n";
    //     throw std::runtime_error("Failed to load texture");
    // }

    // // Formátum meghatározása és szükség esetén konvertálás
    // Uint32 sdl_format = surf->format->BytesPerPixel == 3
    //                         ? SDL_PIXELFORMAT_RGB24
    //                         : SDL_PIXELFORMAT_RGBA32;
    // GLenum source_format = surf->format->BytesPerPixel == 3 ? GL_RGB :
    // GL_RGBA; if (surf->format->format != sdl_format) {
    //     SDL_Surface *formattedSurf =
    //         SDL_ConvertSurfaceFormat(surf, sdl_format, 0);
    //     SDL_FreeSurface(surf);
    //     if (formattedSurf == nullptr) {
    //         std::cout << "Error converting image format: " << SDL_GetError()
    //                   << std::endl;
    //         return;
    //     }
    //     surf = formattedSurf;
    // }

    // // Áttérés SDL koordinátarendszerről ( (0,0) balfent ) OpenGL
    // // textúra-koordinátarendszerre ( (0,0) ballent )
    // if (transform_image(surf->pitch, surf->h, surf->pixels) == -1) {
    //     std::cout << "Error transforming image: " << SDL_GetError()
    //               << std::endl;
    //     SDL_FreeSurface(surf);
    //     return;
    // }

    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(
        GL_TEXTURE_2D, // melyik binding point-on van a textúra
                       // erőforrás, amihez tárolást rendelünk
        0,             // melyik részletességi szint adatait határozzuk meg
        glGpuType,     // textúra belső tárolási formátuma (GPU-n)
        img.size().width(),  // szélesség
        img.size().height(), // magasság
        0,                   // nulla kell, hogy legyen (
           // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glTexImage2D.xhtml
           // )
        glCpuOrder, // forrás (=CPU-n) formátuma
        glCpuType,  // forrás egy pixelének egy csatornáját
                    // hogyan tároljuk
        mem);       // forráshoz pointer

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // SDL_FreeSurface(surf);
    free(mem);

    std::cerr << "created texture " << id << " from file: " << path << "\n";
}

Texture2D::Texture2D(Texture2D &&other) {
    id = other.id;
    internal_format = other.internal_format;
    other.id = GL_NONE; // == 0
}

Texture2D &Texture2D::operator=(Texture2D &&other) {
    std::cerr << "replacing texture " << id << " with " << other.id << "\n";
    std::swap(id, other.id); // other's destructor will free this texture
    internal_format = other.internal_format;
    return *this;
}

Texture2D::~Texture2D() {
    if (id != GL_NONE) {
        std::cerr << "deleting texture " << id << "\n";
        glDeleteTextures(1, &id);
    }
}

void Texture2D::SetFiltering(GLenum min, GLenum mag) {
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture2D::SetWrapping(GLenum s, GLenum t) {
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, s);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, t);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture2D::UploadData(GLenum sourceFormat, GLenum sourceDataType,
                           size_t width, size_t height, void *data) {
    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0,
                 sourceFormat, sourceDataType, data);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture2D::GenerateMipMaps() {
    glBindTexture(GL_TEXTURE_2D, id);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
}

Texture2D::operator GLuint() { return id; }

inline int transform_image(int pitch, int height, void *image_pixels) {
    int index;
    void *temp_row;
    int height_div_2;

    temp_row = (void *)malloc(pitch);
    if (NULL == temp_row) {
        SDL_SetError("Not enough memory for image inversion");
        return -1;
    }
    // if height is odd, don't need to swap middle row
    height_div_2 = (int)(height * .5);
    for (index = 0; index < height_div_2; index++) {
        // uses string.h
        memcpy((Uint8 *)temp_row, (Uint8 *)(image_pixels) + pitch * index,
               pitch);

        memcpy((Uint8 *)(image_pixels) + pitch * index,
               (Uint8 *)(image_pixels) + pitch * (height - index - 1), pitch);
        memcpy((Uint8 *)(image_pixels) + pitch * (height - index - 1), temp_row,
               pitch);
    }
    free(temp_row);
    return 0;
}
