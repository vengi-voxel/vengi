# Purpose

This tool parses GLSL shader files (mainly *.vert, *.frag) and generated C++ source files for them.

# Description

The generator uses `ShaderTemplate.h.in` and `UniformBufferTemplate.h.in` and generates the files by doing some string replacements.

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
