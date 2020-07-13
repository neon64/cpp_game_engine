# C++ Game Engine

## Aims:

Very much a hobby project, am not trying to achieve the best graphics, or world-class performance, or latest features (e.g.: ray-tracing or real-time global illumination).

Instead my aims are as follows:

- adopt a stateless API which verifies as much as possible at compile-time using templates/C++20 concepts (inspired by projects like [Vulkano](https://github.com/vulkano-rs/vulkano), [glium](https://github.com/glium/glium))
- leave open the possibility of porting to Vulkan in the future (though beginning with OpenGL 4+ because I don't want to think about manual memory management and synchronisation of Vulkan objects)
- implement a render-graph system
- slightly improve upon the graphical features of my previous Java game engine, e.g.: transitioning from forward phong shading -> deferred PBR pipeline

My aims are not:

- implement game physics from scratch (I will probably just link to Bullet)
- compete with Unity/Unreal etc... - I don't want to build a level editor or include a scripting API, this will be a (mostly) statically-linked, compiled C++ application.

![](/screenshots/bunnies_and_block.jpg)

^ This screenshot suggests I should really implement tonemapping next.

## Why C++?

TLDR: I would have written it in Rust, but I made a conscious decision to use C++ instead, in order to force me to finally learn it.

1. To learn (modern) C++.
   I have been liberal with my use of new features from C++11 onwards (variant, optional, span, string_view, move semantics, perhaps concepts in the future).

2. Low-level control - trying to achieve higher performance than the Java 3D game engine I worked on many years ago.

3. Ease of interoperability with other libraries

## Building

Uses C++ 20 features - so probably need a relatively recent compiler. I've only tried it with GCC 10 on Linux.

**Dependencies:**

I would love to be lectured about how to do package management properly in C++, as I'm really missing Rust's Cargo package manager at the moment. I installed deps with vcpkg.

- `glfw3` - windowing
- `glm` - matrices and maths for OpenGL
- `glslang` - for offline shader compilation/verification/introspection
    - installed as a submodule at the moment as the vcpkg version doesn't work for me yet
- `assimp` - model-loading
- `fmt` - for `std::format` replacement
- `loguru` - a tiny logging library


## Todo

- deferred-rendering pipeline
- glTF loading
- render graph, draw call sorting: http://realtimecollisiondetection.net/blog/?p=86
