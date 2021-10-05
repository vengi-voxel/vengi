# Configuration

## Variables

The engine can get configured by so called cvars (configuration variables). These variables can be modified from within
the game via key bindings, in-game console, the ui or scripts.

To get a list of supported cvars (they might differ from application to application), type the command `cvarlist` to the
in-game console (`CTRL+Tab` in the default binding).

The variables can get their initial value from various sources. The highest order is the command line. If you specify it on
the command line, every other method will not be used. If the engine finds the cvar name in your environment variables, this
one will take precendence over the one the is found in the configuration file. Next is the configuration file - this one will
take precendence over the default settings that are specified in the code.

The environment variable can be either lower case or upper case. For example it will work if you have `CL_WIDTH` or `cl_width`
exported. The lower case variant has the higher priority.

### Commandline

```bash
./vengi-voxvonvert -set voxformat_scale 2.0 [...]
```

### Environment

```bash
export VOXFORMAT_SCALE=2.0
./vengi-voxconvert [...]
```

### Configuration file

* Linux: `~/.local/share/vengi/voxconvert/voxconvert.vars`
* Windows: `C:/Users/bob/AppData/Roaming/vengi/voxconvert/voxconvert.vars`
* Mac: `/Users/bob/Library/Application Support/vengi/voxconvert/voxconvert.vars`

## Commands

To get a list of supported commands (they might differ from application to application), type the command `cmdlist` to the
in-game console (`CTRL+Tab` in the default binding).

You can also get a list when doing `./vengi-app --help` on the command line.

## Key bindings

You can also modify or add key bindings to commands. Type `bindlist` to the console to get a list of the current active bindings
(and also here: they might differ from application to application). The command `bind` can be used to configure keybindings on-the-fly. These bindings are saved to a file on shutdown.

## Logging

You can either log via syslog (on unix) or to stdout (this might of course differ from platform to platform).

The log level is configured by the `core_loglevel` variable. The lower the value, the more you see. `1` is the highest log level
(trace), where 5 is the lowest log level (fatal error).

## General

To get a rough usage overview, you can start an application with `--help`. It will print out the commands and configuration variables
with a description and hints how to modify/use them.

## Video settings

* `cl_vsync`: enable or disable v-sync
* `cl_gamma`: tweak the gamma value that is applied last on rendering
