project('stereo_simulation', 'cpp', default_options:['warning_level=3','cpp_std=c++17'])

cmake = import('cmake')

# Configure the CMake project
nfd_sub_proj = cmake.subproject('nativefiledialog-extended')
#message('CMake targets:\n - ' + '\n - '.join(nfd_sub_proj.target_list()))
# Fetch the dependency object
nfd = nfd_sub_proj.dependency('nfd')


cpp = meson.get_compiler('cpp')

sdl2 = dependency('sdl2', 'pkg-config')
sdl2_image = dependency('sdl2_image', 'pkg-config')
glew = dependency('glew')
glm = dependency('glm')
gl = dependency('GL')
imgui = dependency('imgui')

magick_args_inc_and_flag = run_command('Magick++-config', 
'--cppflags').stdout().strip().split()
magick_args_lib = run_command('Magick++-config', 
'--ldflags').stdout().strip().split()
magick_args_lib += ['-Bstatic']
add_project_arguments(magick_args_inc_and_flag, language: 'cpp')
add_project_arguments(magick_args_lib, language: 'cpp')
add_project_link_arguments(magick_args_lib, language: 'cpp')

# miniwrapper_incl = include_directories('/common/tovabbi_szortirozando/e/hobbi/miniwrapper/includes')
#miniwrapper_lib = static_library('miniwrapper')
#miniwrapper = declare_dependency(link_with : miniwrapper_lib, include_directories : miniwrapper_incl)

#miniwrapper = dependency('miniwrapper', method: 'system')
# miniwrapper = cpp.find_library('miniwrapper', static: true, dirs:['/usr/local/lib'], has_headers: ['miniwrapper.hpp'],header_include_directories:miniwrapper_incl)
# mwdep = declare_dependency(dependencies: [miniwrapper], include_directories: miniwrapper_incl)

mw_proj = subproject('miniwrapper')
mw_dep = mw_proj.get_variable('miniwrapper_dep')

sources=['main.cpp', 'app.cpp', 'camera.cpp', 'GLDebugMessageCallback.cpp', 'calibration.cpp', 'textures.cpp', 'renderstep.cpp', 'normalsdisplay.cpp', 'cross_normal.cpp', 'affine_normal.cpp', 'affine.cpp', 'comparison.cpp', 'textureviewer.cpp', 'noiseadder.cpp', 'transformer.cpp', 'd2nt.cpp', 'measure.cpp', 'sda.cpp', 'pca.cpp', 'save_texture.cpp', 'canny.cpp', 'testconfig.cpp']
dependencies = [sdl2,sdl2_image,glew,glm,gl,mw_dep,imgui,nfd]

executable('stereo_simulator',
           sources: sources,
           install : true,
           #include_directories: [nfd_incl],
           dependencies : dependencies)