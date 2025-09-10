#pragma once

#include <GL/glew.h>
#include <vector>

template <typename T> class SSBO {
  public:
    SSBO<T>(const std::vector<T> data, GLenum usage = GL_STATIC_DRAW);
    ~SSBO<T>();

    SSBO<T>(const SSBO<T> &other) = delete;
    SSBO<T>(SSBO<T> &&other);
    SSBO<T>& operator=(SSBO<T> &&other);

    operator GLuint();

    void Bind(int binding_index);
    void UnBind();

  private:
    GLuint id = 0;
};

template <typename T> SSBO<T>::SSBO(const std::vector<T> data, GLenum usage) {
    glGenBuffers(1, &id);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, id);
    glBUfferData(GL_SHADER_STORAGE_BUFFER, sizeof(T) * data.size(), data.data(),
                 usage);
    // bind buffer base? -
    // https://www.khronos.org/opengl/wiki/Shader_Storage_Buffer_Object
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

template <typename T> SSBO<T>::~SSBO() {
    if (id != GL_NONE) {
        glDeleteBuffers(1, &id);
    }
}

template <typename T> SSBO<T>::SSBO(SSBO<T> &&other) {
    id = other.id;
    other.id = GL_NONE;
}

template <typename T> SSBO<T>& SSBO<T>::operator=(SSBO<T> &&other) {
    std::swap(id, other.id); // other's destructor will still run
    return *this;
}

template <typename T> SSBO<T>::operator GLuint() { return id; }

template <typename T> void SSBO<T>::Bind(int binding_index) {
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding_index, id);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, id);
}

template <typename T> void SSBO<T>::UnBind() {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}