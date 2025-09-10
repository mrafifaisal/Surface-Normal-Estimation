#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl2.h>
#include <miniwrapper.hpp>
#include <Magick++.h>

#include <fstream>
#include <vector>

SDLWindow::SDLWindow(const std::string &name, const int sizex,
                     const int sizey) {
    failed = false;

    handle = SDL_CreateWindow(
        name.c_str(), // az ablak fejléce
        100,          // az ablak bal-felső sarkának kezdeti X koordinátája
        100,          // az ablak bal-felső sarkának kezdeti Y koordinátája
        sizex,        // ablak szélessége
        sizey,        // és magassága
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN |
            SDL_WINDOW_RESIZABLE); // megjelenítési tulajdonságok

    if (handle == nullptr) {
        std::cout
            << "[Window creation] Error during the creation of an SDL window: "
            << SDL_GetError() << std::endl;
        failed = true;
        return;
    }

    context = SDL_GL_CreateContext(handle);
    if (context == nullptr) {
        std::cout << "[OGL context creation] Error during the creation of the "
                     "OGL context: "
                  << SDL_GetError() << std::endl;
        failed = true;
        return;
    }

    if (context == nullptr) {
        failed = true;
        return;
    }

    SDL_GL_SetSwapInterval(1);

    GLenum error = glewInit();
    if (error != GLEW_OK) {
        std::cout << "[GLEW] Error during the initialization of glew:"
                  << std::endl;
        failed = true;
        return;
    }

    int glVersion[2] = {-1, -1};
    glGetIntegerv(GL_MAJOR_VERSION, &glVersion[0]);
    glGetIntegerv(GL_MINOR_VERSION, &glVersion[1]);
    std::cout << "Running OpenGL " << glVersion[0] << "." << glVersion[1]
              << std::endl;

    if (glVersion[0] == -1 && glVersion[1] == -1) {
        SDL_GL_DeleteContext(context);
        SDL_DestroyWindow(handle);

        std::cout << "[OGL context creation] Error during the inialization of "
                     "the OGL context! Maybe one of the "
                     "SDL_GL_SetAttribute(...) calls is erroneous."
                  << std::endl;

        failed = true;
        return;
    }

    GLint context_flags;
    glGetIntegerv(GL_CONTEXT_FLAGS, &context_flags);
    if (context_flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE,
                              GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr,
                              GL_FALSE);
        glDebugMessageCallback(GLDebugMessageCallback, nullptr);
    }
}

SDLWindow::~SDLWindow() {
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(handle);
}

SDLWindow::operator SDL_Window *() const { return handle; }
SDLWindow::operator SDL_GLContext() const { return context; }

bool SDLWindow::failed_to_initialize() const { return failed; }

bool InitSDLGL(bool debug) {
    if (SDL_Init(SDL_INIT_VIDEO) == -1) {
        // irjuk ki a hibat es terminaljon a program
        std::cout
            << "[SDL initialization] Error during the SDL initialization: "
            << SDL_GetError() << std::endl;
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_CORE);
//#ifdef _DEBUG
    // ha debug módú a fordítás, legyen az OpenGL context is debug módban, ekkor
    // működik a debug callback
    if(debug){
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
    }
//#endif

    SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 32);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    // Initialize image magick
    Magick::InitializeMagick("/usr");

    return true;
}

bool InitIMGUI(const SDLWindow &window) {
    IMGUI_CHECKVERSION();
    bool success = true;
    success &= ImGui::CreateContext() != nullptr;

    success &= ImGui_ImplSDL2_InitForOpenGL(window, window);
    success &= ImGui_ImplOpenGL3_Init();
    return success;
}
