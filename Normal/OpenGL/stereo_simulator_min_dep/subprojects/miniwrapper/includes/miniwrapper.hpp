#ifndef MINI_WRAPPER
#define MINI_WRAPPER

// GLEW
#include <GL/glew.h>

// SDL
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#include "GLDebugMessageCallback.hpp"
#include <unordered_map>

#include <glm/glm.hpp>
#include <iostream>
#include <string>

// struct GLContext {
//    private:
//     SDL_GLContext context;

//    public:
//     GLContext(SDL_Window* win) {
//         context = SDL_GL_CreateContext(win);
//         if (context == nullptr) {
//             std::cout << "[OGL context creation] Error during the creation of
//             the "
//                          "OGL context: "
//                       << SDL_GetError() << std::endl;
//         }
//     }

//     ~GLContext() {
//         SDL_GL_DeleteContext(context);
//     }

//     operator SDL_GLContext() { return context; }
// };B

struct SDLWindow {
  private:
    SDL_Window *handle;
    SDL_GLContext context;

    bool failed;

  public:
    SDLWindow(const std::string &name, const int sizex = 800,
              const int sizey = 600);

    ~SDLWindow();

    operator SDL_Window *() const;
    operator SDL_GLContext() const;

    bool failed_to_initialize() const;
};

bool InitSDLGL(bool debug);

bool InitIMGUI(const SDLWindow &window);

#endif