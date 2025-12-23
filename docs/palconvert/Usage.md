# Usage

Print a detailed description of the program parameters for your particular version.

```sh
vengi-palconvert --help
```

> You can use the bash completion or zsh completion
>
> Put this in your `.bashrc` or `.zshrc`
>
> `source <(vengi-palconvert --completion bash)` (or replace `bash` by `zsh`)

* `--force`: overwrite existing files
* `--input <file>`: allows to specify input files. You can specify more than one file
* `--optimize`: Optimize the palette by removing duplicated or full transparent colors
* `--output <file>`: allows you to specify the output filename
* `--quantize`: Quantize the input palette to 256 colors
* `--type <type>`: Specify the output type (ansi, json, hex)

## bash completion

You can also use the bash completion script by adding this to your `.bashrc`

```sh
source <(vengi-palconvert --completion bash)
```
