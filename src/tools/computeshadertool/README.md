# CompuseShaderTool

This tool parses OpenCL shader files (`*.cl`) and generates C++ source files for them.

# Description

The generator uses `ComputeShaderTemplate.h.in` and generates the files by doing some string replacements.

There are several variables in the template file that are replaced by the generator.
* `$includes$`
* `$namespace$`
* `$name$`
* `$createkernels$`
* `$shutdown$`
* `$kernels$`

The parser includes a preprocessor.
