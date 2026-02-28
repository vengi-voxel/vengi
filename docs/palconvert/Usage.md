# Usage

Print a detailed description of the program parameters for your particular version.

```sh
vengi-palconvert --help
```

> You can use the bash, zsh or powershell completion
>
> Put this in your `.bashrc`, `.zshrc` or PowerShell `$PROFILE`
>
> `source <(vengi-palconvert --completion bash)` (or replace `bash` by `zsh` or `powershell`)

* `--force`: overwrite existing files
* `--input <file>`: allows to specify input files. You can specify more than one file
* `--optimize`: Optimize the palette by removing duplicated or full transparent colors
* `--output <file>`: allows you to specify the output filename
* `--quantize`: Quantize the input palette to 256 colors
* `--type <type>`: Specify the output type (ansi, json, hex)

## Shell completion

### bash

Add this to your `.bashrc`:

```sh
source <(vengi-palconvert --completion bash)
```

### zsh

Add this to your `.zshrc`:

```sh
source <(vengi-palconvert --completion zsh)
```

### PowerShell

Add this to your PowerShell profile (`$PROFILE`):

```powershell
vengi-palconvert --completion powershell | Invoke-Expression
```
