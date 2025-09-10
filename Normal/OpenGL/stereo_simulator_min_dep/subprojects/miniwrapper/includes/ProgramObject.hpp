#pragma once

#include <string>
#include <unordered_map>

#include <GL/glew.h>
#include <glm/glm.hpp>

#include "ShaderObject.hpp"

/// IMPORTANT: adding a proper destructor will break stereo simulator as its
/// rendersteps rely on copying ProgramObjects without freeing them
class ProgramObject {
    GLuint id;

    std::unordered_map<std::string, GLuint> uniform_ids;

    GLuint GetLocation(const std::string &s);

  public:
    ProgramObject(GLuint id);
    ProgramObject(std::initializer_list<ShaderObject> shaders);

    void Use();

    void Unuse();

    void SetUniform(const std::string &name, glm::vec4 val);

    void SetUniform(const std::string &name, glm::vec3 val);

    void SetUniform(const std::string &name, glm::vec2 val);

    void SetUniform(const std::string &name, float val);

    void SetUniform(const std::string &name, double val);

    void SetUniform(const std::string &name, int val);

    void SetUniform(const std::string &name, GLuint val);

    void SetUniform(const std::string &name, glm::ivec2 val);

    void SetUniform(const std::string &name, glm::vec3 *val, int count);

    void SetUniform(const std::string &name, glm::mat4 val);

    void SetUniform(const std::string &name, float *val, int count);

    void SetUniform(const std::string &name, glm::vec2 *val, int count);

    void SetUniform(const std::string &name, glm::vec4 *val, int count);

    void SetTexture(const std::string &name, int id, GLuint texture);

    void SetImage(const std::string &name, int id, GLuint texture);
};