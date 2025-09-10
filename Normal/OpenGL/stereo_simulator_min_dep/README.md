# Environment

This project was developed on Manjaro, an arch based linux distribution.
However for the sake of convenience, package names are provided for debian based distributions below.
Building and running the project was tested on Ubuntu 24.04 LTS as well.
Below are the instructions for building and running the project on Ubuntu or other Debian based distributions.

# Building

Ensure GPU drivers are installed.

Install the following tools & dependencies:
- meson (+ninja)
- cmake
- pkgconf
- GTK3 for native file dialog: libgtk-3-dev
- SDL2 for window management and image loading: libsdl2-dev, libsdl2-image-dev
- GLEW for handling OpenGL: libglew-dev
- OpenGL mathematics: libglm-dev
- ImGUI: libimgui-dev
- ImageMagick for loading floating poitn image formats: imagemagick<br>
Note: this program was developed with imagemagick 7, so it might be incompatible with imagemagick 6 <br>
Note: on debian based distributions only imagemagick 6 is available in the main repository
You may need to install it from the unstable/testing repository or from source


In the main directory where the meson.build configuration file is located run the following commands:

1. `meson setup builddir`

2. `cd builddir`

3. `ninja`

OR execute after `build.sh` meson has been set up

# Running
From the main directory:

`./builddir/stereo_simulator`

OR execute `run.sh` (this will build the project as well)

Note: depending on the GPU driver some warnings & errors might be emitted on the console when running the program.

# Basic usage

This project is primarily a TDK thesis and research project. Therefore ease of use, failproof ui and extensive documentation was not a priority. 

*A brief introduction is given below*

The two most important windows for trying out the proposed method and a few other normal estimators are the **Data** and **Rendersteps** windows.

## Data window

Here you may load input textures or adjust the parameters of the simulation.

The top radio buttons control switching between the simulation and loaded input data.

**Caution:** All used input textures must be loaded first before switching to "loaded disparities" with the radio button, otherwise the program will crash.

Below are some camera parameters for the visualization (free) camera. (speed, fov, background, transform reset)
(The one used for rendering the pointcloud in a 3D view for display)

This camera may be controlled the WASD keys for movement. For looking around hold down the left mouse button and while moving the mouse. Hold CTRL for more speed and SHIFT for less.

There are two "pages" for simulation and loading settings.

### Simulation page

Here you may select the scene:
- *3 meshes* (tetrahedron, cube, approx spehere)
- *stability test* contains a single plane with an adjustable angle used for testing numeric precision errors.
- *sphere* contains an analytic (exact) sphere rendered with raycasting, in front of a plane. This was used to test the effect of disparity noise depending on the relative angle of the surface to the camera.
- *discontinuity* is a scene containing an arrangement of many spheres, tetrahedrons and boxes. This was primarily used for visualizing the effect of different stopping conditions as well as the effect of distance and noise.

***Virtual camera***

The virtual camera simulates a pair of stereo cameras with ground truth pixel disparities, normals, depth and colors calculated. Its settings are:
- resolution
- baseline - the distance between the cameras (in mm)
- position and target to look at (both are only set when the "set!" button is pressed)

Additionally when the "Control stereo camera" checkbox is ticked the normal camera controls (keyboard + mouse) are used for moving the stereo camera around instead of the free flying camera.
This makes the most sense when the free camera is set to be in its default position looking exactly forward from the origin.

### Loaded page
This page is used for loading in different input and ground truth textures for the program to use instead of the simulation data.

There are also buttons for loading a set of test images from different datasets as an example.

## Rendersteps window
The entire programs rendering is modular in the snese that the input of any shader (except the simulation shaders) can be set to the output of most other shaders.

There are two kinds of rendersteps (displayed in two pages on the UI):
- intermediate steps (used for calculations on the gpu, eg the normal estimation)
- display steps (these steps directly write to the output framebuffer displayed on the screen)

Each frame the enabled intermediate steps are executed in top-to-bottom order. Then they are followed by the display steps in the same order.

Because of the execution order, each step should only use previously executed steps as its input. Otherwise if the scene is changing we get slightly incorrect results. In case of stationary scenes any order produces correct results after a few frames if cycles are avoided.

### Description of intermediate steps

- **Transform** - basic image manipulation (color transform, flip)
- **Noise adder** - adds gaussian noise to its input, mainly used for adding noise to disparities
    - *passthrough:* do not add noise, just write the input to the output
    - *realtime*: regenerate the noise texture each frame on the cpu (hurts performance)
- **Triangulation** - triangulate world positions from disparities, with optional scaling
- **Canny** - Canny edge detector implementation
- **Laplacian** - Laplacian filter
- **Gradiant** - either compute gradiant or optionally compute a discontinuity aware gradiant based on the D2NT paper
- **Affine parameter estimator** - Estimates affine transformation parameters between each pixel pair's regions
    - *basic* - this mode only uses two neighboring pixels (essentially the same as computing cross normal)
    - *lsq* - Least squares optimal approximation as described in the paper, the kernel is a fix square
    - *threshold* - Adaptive version with a simple threshold value as a stopping condition. Uses the "edges" input to determine when to stop.
    - *cumulative* - Adaptive version with cumulative error from "edges" input, stops at given threshold.
    - *covered depth* - Adaptive method with stopping based on the covered depth of each traversal, relative to the depth of the central pixel.
    - *angle* - Adaptive version, here the angle is computed on each three point segment. Stops when the world space angle reaches a given threshold.
- **Cross normal estimator** - Computes the normal from center and neighboring world coordinates with option for taking the symmetric difference.
- **Affine normal estimator** - Computes normal from affine parameters
- **D2NT** - Another method for estimating the normal from the paper "D2NT: A High-Performing Depth-to-Normal Translator" (2023)
- **SDA-SNE** - A supposedly robust normal estimator from the paper "SDA-SNE: Spatial Discontinuity-Aware Surface Normal Estimation via Multi-Directional Dynamic Programming"<br>
This is a failed attempt at porting their python/matlab implementation to a GPU shader. Even though the results are incorrect, the performance should be indicative of what a correct implementation should be able to achieve.
- **PCA** - Principle component analysis based normal estimator
- **MRF Refiner** - A post process normal refiner from the D2NT paper. Unfortunately on noisy data it produces only negligable improvements.
- **Texture compare** - Compare two textures, based on various norms and measures.
    - Euclidean
    - Manhattan
    - Plain difference
    - Angle between
    - *Gradient map* - use multiple colors on the scale for better visibility.
    - *Mask* use a mask texture for avoiding areas outside the target object.
- **Measure error** - An obsolate tool for displaying the error in realtime

### Description of display steps
- **Point cloud visualizer** - Draws a colored pointcloud from the view of the free camera
    - *display only nth* - Use this to only draw a fraction of the points in case visibility or performance is bad.
- **Normal visualizer** - Draw normals as line segments from the view of the free camera.
    - *mask* - use this to only draw normals where they were computed (eg.: ignore background on sphere test)
    - *display only nth* - Use this to only draw a fraction of the normals in case visibility or performance is bad.
    - *length*
    - *screen space length* - ensure all drawn normals are the same given length on the screen. Useful for figures.
    - *color*
    - *Both sides* - draw normal in both direction from the center point (normal estimators produce ambigous results as they are unaware of which of the two directions the surface faces)
    - *Smart invert* - this option emplys some heuristics to make most normals face the camera
    - *anti-aliasing* - enable anti-aliasing when drawing the line
- **Texture viewer** - Display the given texture directly. Very useful for debugging, especially along with *Texture compare*.

## Default configuration

By default the program simulates a scene with three primitives and visualizes the pointcloud and the normals. Normals are calculated with the affine estimator but with the *basic* option only using two neighbors. There is no added noise by default.

In order to try other affine methods, it is enough to change the options under the intermediate render step **Affine parameter estimator**. For other methods it is necessary to reassign the input texture used in the **Normal visualizer**.