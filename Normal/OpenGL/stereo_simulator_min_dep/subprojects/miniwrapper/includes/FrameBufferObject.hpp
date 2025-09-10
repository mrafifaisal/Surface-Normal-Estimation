#pragma once

#include <GL/glew.h>
#include <initializer_list>
#include <utility>

#include "Texture2D.hpp"

class FrameBufferObject {
  public:
    FrameBufferObject(
        std::initializer_list<std::pair<GLenum, Texture2D&>> textureAttachments);
    FrameBufferObject(FrameBufferObject&& other);
    ~FrameBufferObject();

    FrameBufferObject& operator=(FrameBufferObject&& other);
    
    void Bind();

    operator GLuint();

  private:
    GLuint id;
};