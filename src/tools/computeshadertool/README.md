# Purpose

This tool parses OpenCL shader files (*.cl) and generated C++ source files for using them.

The generator uses `ComputeShaderTemplate.h.in` and generate the files on base of them.

There are several variables in the template file that are replaced by the generator.
* `$includes$`
* `$namespace$`
* `$name$`
* `$createkernels$`
* `$shutdown$`
* `$kernels$`
