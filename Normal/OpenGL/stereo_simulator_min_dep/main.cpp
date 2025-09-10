#include <iostream>
#include <vector>

#include <SDL2/SDL.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform2.hpp>

#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl2.h>
#include <imgui_internal.h>

#include <nfd.h>

#include <miniwrapper.hpp>

#include "app.hpp"

#define _DEBUG
#include "GLDebugMessageCallback.h"

#include <unistd.h>

int main(int argc, char *argv[]) {
    NFD_Init();

    char buf[1024];
    getcwd(buf, 1024);
    std::cout << buf << "\n";

    if (!InitSDLGL(true)) {
        std::cerr << "Failed to initialize SDL/GL (miniwrapper)\n";
        exit(1);
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);

    SDLWindow window("Stereo simulator", 800, 600);
    if (window.failed_to_initialize()) {
        std::cerr << "Failed to initialize window\n";
        exit(1);
    }

    std::string vendor = (char *)glGetString(GL_VENDOR);
    std::string renderer = (char *)glGetString(GL_RENDERER);
    std::cout << "Vendor: " << vendor << "\nRnderer: " << renderer << "\n";

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE,
                          GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
    glDebugMessageCallback(GLDebugMessageCallback, nullptr);

    InitIMGUI(window);

    ImGui::GetIO().PlatformLocaleDecimalPoint = *localeconv()->decimal_point;

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    int bits;
    glGetQueryiv(GL_TIME_ELAPSED, GL_QUERY_COUNTER_BITS, &bits);
    std::cout << "time measurement precision: " << bits << " bits\n";
    
    glClearColor(0.0f, 0.1f, 0.2f, 1.0f);
    {
        App app;
        std::cout<<"Initialization complete\n";

        if(argc > 1){
            
            TestConfig config;
            config.disparity_file = std::string(argv[1]);
            config.calibration_file = std::string(argv[2]);
            config.gtnormal_fila = std::string(argv[3]);
            config.mask_file = std::string(argv[4]);
            config.method = 0;
            config.affine_mode = 1;
            config.conv_size = 9;
            config.adaptive_dir_count = 16;
            config.adaptive_max_step_count = 5;
            config.adaptive_depth_thresh_ratio = 0.02;

            if(argc > 5){
                std::cout<<"Reading test config\n";
                std::string prev_loc = std::setlocale(LC_ALL, nullptr);
                std::setlocale(LC_ALL, "en_US.UTF-8");
                config.ReadParams(argv[5]);
                std::setlocale(LC_ALL, prev_loc.c_str());
            }

            std::cout<<"Starting test with config:\n";
            config.PrintConfig();
            app.RunSingleTest(config, window);
            return 0;
        }

        bool quit = false;
        bool fullscreen = false;
        while (!quit) {
            SDL_Event event;
            bool mouse_captured = ImGui::GetIO().WantCaptureMouse;
            bool keyboard_captured = ImGui::GetIO().WantCaptureKeyboard;
            while (SDL_PollEvent(&event)) {
                ImGui_ImplSDL2_ProcessEvent(&event);
                switch (event.type) {
                case SDL_QUIT:
                    quit = true;
                    break;
                case SDL_WINDOWEVENT:
                    if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                        app.Resize(event.window.data1, event.window.data2);
                    }
                    break;
                case SDL_KEYDOWN:
                    if (event.key.keysym.sym == SDLK_F11) {
                        fullscreen = !fullscreen;
                        SDL_SetWindowFullscreen(
                            window,
                            fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
                        break;
                    }

                    if (!keyboard_captured)
                        app.OnKeyDown(event.key);
                    break;
                case SDL_KEYUP:
                    if (!keyboard_captured)
                        app.OnKeyUp(event.key);
                    break;
                case SDL_MOUSEMOTION:
                    if (!mouse_captured)
                        app.OnMouseMove(event.motion);
                    break;
                }
            }
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplSDL2_NewFrame();
            ImGui::NewFrame();

            app.Update();
            app.Render();
            app.DrawGUI();
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            SDL_GL_SwapWindow(window);
        }
    }

    NFD_Quit();
}