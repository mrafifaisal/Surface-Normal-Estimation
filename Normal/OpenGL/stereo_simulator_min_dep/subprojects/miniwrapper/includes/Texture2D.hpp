#pragma once

#include <GL/glew.h>
#include <string>

class Texture2D {
  public:
    Texture2D(size_t width, size_t height, GLenum format = GL_RGBA8,
              GLenum srcFormat = GL_RGBA, GLenum type = GL_UNSIGNED_BYTE);
    Texture2D(std::string path, bool normalized = false);
    Texture2D(Texture2D &&other);
    ~Texture2D();

    Texture2D &operator=(Texture2D &&other);

    void SetFiltering(GLenum min, GLenum mag);
    void SetWrapping(GLenum s, GLenum t);
    void UploadData(GLenum sourceFormat, GLenum sourceDataType, size_t width,
                    size_t height, void *data);
    void GenerateMipMaps();

    operator GLuint();

  private:
    GLuint id;
    GLenum internal_format;
};