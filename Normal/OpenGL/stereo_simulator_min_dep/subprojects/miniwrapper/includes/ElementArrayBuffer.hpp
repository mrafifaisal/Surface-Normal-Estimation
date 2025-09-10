#pragma once

#include <GL/glew.h>
#include <initializer_list>
#include <vector>

template <typename T = GLuint> class ElementArrayBuffer {
  public:
    ElementArrayBuffer<T>(const std::vector<T> &data,
                          GLenum usage = GL_STATIC_DRAW,
                          GLenum idx_type = GL_UNSIGNED_INT);
    ~ElementArrayBuffer<T>();

    operator GLuint();
    void Bind() const;

    using id_type = T;

    GLenum GetIdxType() const { return idx_type_opengl; };

  private:
    GLuint id;
    GLuint idx_type_opengl;
};

template <typename T>
ElementArrayBuffer<T>::ElementArrayBuffer(const std::vector<T> &data,
                                          GLenum usage, GLenum idx_type)
    : idx_type_opengl(idx_type) {
    glGenBuffers(1, &id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(T) * data.size(), data.data(),
                 usage);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

template <typename T> ElementArrayBuffer<T>::~ElementArrayBuffer() {
    glDeleteBuffers(1, &id);
}

template <typename T> ElementArrayBuffer<T>::operator GLuint() { return id; }

template <typename T> void ElementArrayBuffer<T>::Bind() const {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
}