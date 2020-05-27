# ShaderTool

This tool parses GLSL shader files (mainly `*.vert`, `*.frag`, `*.geom` and `*.comp`) and generates C++ source files for them.

The cmake macros expect the shader source below the module in a `shaders/` directory.

```cmake
set(SHADERS first second)
generate_shaders(mymodulename ${SHADERS})
```

The shaders given in this example would be located at `src/modules/mymodulename/shaders/first.*`. The tool automatically detects the type of programs that should be connected in the final shader.

The code is generated into the build directory in `generated/shaders`.

## Description

The generator uses `ShaderTemplate.h.in` and `UniformBufferTemplate.h.in` as a base to generat the C++ source files.

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
