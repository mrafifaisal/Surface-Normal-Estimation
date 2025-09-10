#include "FrameBufferObject.hpp"

#include <iostream>
#include <vector>

FrameBufferObject::FrameBufferObject(
    std::initializer_list<std::pair<GLenum, Texture2D &>> textureAttachments) {
    glGenFramebuffers(1, &id);
    glBindFramebuffer(GL_FRAMEBUFFER, id);

    std::vector<GLenum> colorAttachments;

    for (auto attachment : textureAttachments) {
        glBindTexture(GL_TEXTURE_2D, attachment.second);
        attachment.second.SetFiltering(GL_NEAREST, GL_NEAREST);
        attachment.second.SetWrapping(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, attachment.first, GL_TEXTURE_2D,
                               attachment.second, 0);
        if (glGetError() != GL_NO_ERROR) {
            std::cout << "Error creating attachment " << attachment.first
                      << std::endl;
            // exit(1);
        }

        if (attachment.first >= GL_COLOR_ATTACHMENT0 &&
            attachment.first <= GL_COLOR_ATTACHMENT15)
            colorAttachments.push_back(attachment.first);
    }

    glDrawBuffers(colorAttachments.size(), colorAttachments.data());

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Incomplete framebuffer (";
        switch (status) {
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
            std::cerr << "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
            std::cerr << "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
            break;
        case GL_FRAMEBUFFER_UNSUPPORTED:
            std::cerr << "GL_FRAMEBUFFER_UNSUPPORTED";
            break;
        }
        std::cerr << ")" << std::endl;
        // exit(1);
    }
}

FrameBufferObject::FrameBufferObject(FrameBufferObject &&other) {
    this->id = other.id;
    other.id = GL_NONE;
}

FrameBufferObject::~FrameBufferObject() {
    if (id != GL_NONE) {
        std::cerr << "deleting framebuffer " << id << "\n";
        glDeleteFramebuffers(1, &id);
    }
}

FrameBufferObject &FrameBufferObject::operator=(FrameBufferObject &&other) {
    std::cerr << "replacing framebuffer " << id << " with " << other.id << "\n";
    std::swap(id, other.id);
    return *this;
}

void FrameBufferObject::Bind() { glBindFramebuffer(GL_FRAMEBUFFER, id); }

FrameBufferObject::operator GLuint(){
    return id;
}