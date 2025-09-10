#pragma once

#include <GL/glew.h>

#include "ArrayBuffer.hpp"
#include "ElementArrayBuffer.hpp"

class VertexArrayObject {
  public:
    VertexArrayObject();
    ~VertexArrayObject();

    template <typename T>
    void AddAttribute(const ArrayBuffer<T> &buffer, GLuint idx,
                      GLuint components, GLenum dataType, GLenum normalized,
                      std::size_t stride, std::size_t offset) {
        Bind();
        buffer.Bind();
        glEnableVertexAttribArray(idx);
        glVertexAttribPointer(idx, components, dataType, normalized,
                              (GLuint)stride, (void *)offset);
        glBindVertexArray(0);
    }

    template <typename T> void AddIndexBuffer(const ElementArrayBuffer<T> &ib) {
        Bind();
        ib.Bind();
        glBindVertexArray(0);
    }

    void Bind() const;
    void Unbind() const;

  private:
    GLuint id;
};