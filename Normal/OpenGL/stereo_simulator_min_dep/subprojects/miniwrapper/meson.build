project('miniwrapper', 'cpp', default_options:['warning_level=3','cpp_std=c++17'])

sdl2 = dependency('sdl2')
sdl2_image = dependency('sdl2_image')
glew = dependency('glew')
glm = dependency('glm')
gl = dependency('GL')
magick_args_inc_and_flag = run_command('Magick++-config', 
'--cppflags').stdout().strip().split()
magick_args_lib = run_command('Magick++-config', 
'--ldflags').stdout().strip().split()
magick_args_lib = ['-Bstatic'] + magick_args_lib
# magick_args_lib = ['-Bstatic', '-lMagick++-7.Q16HDRI', '-lMagickWand-7.Q16HDRI', '-lMagickCore-7.Q16HDRI']
add_project_arguments(magick_args_inc_and_flag, language: 'cpp')
add_project_link_arguments(magick_args_lib, language: 'cpp')

dependencies = [sdl2,sdl2_image,glew,glm,gl]
sources = ['src/GLDebugMessageCallback.cpp', 
            'src/miniwrapper.cpp', 
            'src/imgui_impl_opengl3.cpp', 
            'src/imgui_impl_sdl2.cpp',
            'src/ArrayBuffer.cpp',
            'src/VertexArrayObject.cpp',
            'src/ProgramObject.cpp',
            'src/ShaderObject.cpp',
            'src/FrameBufferObject.cpp',
            'src/Texture2D.cpp',
            'src/Mesh.cpp'
          ]

miniwrapper_lib = static_library('miniwrapper',
           sources: sources,
           install : true,
           include_directories: ['includes'],
           dependencies : dependencies)

miniwrapper_dep = declare_dependency(include_directories: include_directories('includes'), link_with: miniwrapper_lib)