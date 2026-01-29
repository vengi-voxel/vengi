# ShaderTool

This tool parses GLSL shader files (mainly `*.vert`, `*.frag`, `*.geom` and `*.comp`) and generates C++ source files for them.

> You automatically get the shaders added back to the code after saving a shader file and trigger a re-build.

The cmake macros expect the shader source below the module in a `shaders/` directory.

```cmake
set(SHADERS first second)
engine_generate_shaders(mymodulename ${SHADERS})
```

The shaders given in this example would be located at `src/modules/mymodulename/shaders/first.*`. The tool automatically detects the type of programs that should be connected in the final shader.

The code is generated into the build directory in `generated/shaders`.

## Description

The generator uses `ShaderTemplate.h.in` and `UniformBufferTemplate.h.in` as a base to generate the C++ source files.

There are several variables in the template file that are replaced by the generator.

* `$includes$`
* `$namespace$`
* `$name$`

* `$setters$`
* `$attributes$`
* `$uniforms$`
* `$uniformarrayinfo$`

* `$shutdown$`
* `$uniformbuffers$`

The parser includes a preprocessor.

You can export constants from the GLSL shader code to the generated C++ code by using `$constant`.

Use `$constant varname 42` to generate a method that returns 42 with a name `getVarname`

## Uniform Buffer Objects (UBO)

Uniform buffer objects allow you to share uniform data between multiple shaders efficiently. The shadertool generates C++ structs that match the GLSL memory layout.

```glsl
layout(std140) uniform MaterialBlock {
    vec4 diffuseColor;
    vec4 specularColor;
    float shininess;
};
```

This generates a `MaterialBlockData` struct with proper std140 alignment and padding. The generated code includes:
- A struct with correct padding for the memory layout
- Methods to create and update the uniform buffer
- Static assertions to verify struct size at compile time

## Shader Storage Buffer Objects (SSBO)

SSBOs allow read/write access to buffer data in shaders. They are parsed using the `buffer` keyword and support the `std430` layout which has tighter packing rules than `std140`.

```glsl
layout(std430, binding = 0) buffer ParticleBuffer {
    vec4 positions[64];
    vec4 velocities[64];
    float masses[];  // Dynamic array (must be last member)
};
```

This generates:
- A `ParticleBufferData` struct with std430 alignment
- A separate header file `YourShaderSSBO.h` with the buffer wrapper class using `video::ShaderStorageBuffer`
- Methods for creating, updating, and binding the buffer
- The binding index as a static constexpr

### Generated SSBO Class

The generated SSBO wrapper class provides:

```cpp
// Create the buffer with initial data
ssbo.create(&data, count);

// Update the buffer
ssbo.update(&data, count);

// Bind to the shader binding point
ssbo.bind();

// Access the underlying video::ShaderStorageBuffer
ssbo.getParticleBufferBuffer();
```

### std430 vs std140 Layout

The key difference between std430 (SSBOs) and std140 (UBOs) is array padding:
- **std140**: Arrays of scalars (float, int) are padded to vec4 boundaries (16 bytes)
- **std430**: Arrays use natural alignment without vec4 padding

```glsl
// std430 example - tighter packing
layout(std430, binding = 1) buffer DataBuffer {
    mat4 transforms[16];
    int count;
    uint flags;
};
```

### Dynamic Arrays

SSBOs support unsized arrays as the last member:

```glsl
layout(std430, binding = 0) buffer DynamicBuffer {
    int fixedData;
    float dynamicArray[];  // Size determined at runtime
};
```

The generated struct uses `[1]` as a placeholder, and you allocate the actual size when creating the buffer.

## Branching / Feature toggles

Usually you don't have to use branching and uniforms for feature toggles. You can use cvars with the flag `CV_SHADER` set. If you are going to change one of these cvars, the shaders are recompiled with the value of the cvar given as preprocessor define.

This means that you can do stuff like:

```glsl
#if cl_shadowmap == 1
   [...]
#else
   [...]
#endif
```

The `#define` of `cl_shadowmap` is done by the shader system at compile time.
