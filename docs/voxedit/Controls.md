# Controls

![voxedit-keybindings](../img/voxedit-keybindings.png)

There are different schemas available to pick from:

- Blender
- Magicavoxel
- Qubicle
- Vengi

See the binding editor in the menu bar to get a complete list of bindings or switch between the keymaps.

You can also manually edit the `keybindings-x.cfg` file in your user settings directory. Go to **About -> Paths** and click the first entry. This will open your user settings directory for voxedit. The file is there if you at least once quit the application.

You can also change the bindings in the console by using the `bind` command.

> Usage of the bind command: `bind <modifier+key> <command> <context>`.
> `context` is one of `all`, `model`, `scene`, `game`, `editing` (`editing` is both `scene` and `model`)

To get a list of bindable commands, type the `cmdlist` command to the console and hit enter.

## Key bindings

### Contexts

| Context   | Description                             |
| --------- | --------------------------------------- |
| `all`     | Available in all modes                  |
| `model`   | Only available in model mode            |
| `scene`   | Only available in scene mode            |
| `game`    | Only available in game mode             |
| `editing` | Available in model, scene and game mode |

You can also negate a context by prefixing it with `!`. For example `!scene` means available in all modes except scene mode.

### Modifiers

| Modifier      | Description                                      |
| :------------ | :----------------------------------------------- |
| `shift`       | Left or right shift                              |
| `left_shift`  | Left shift                                       |
| `right_shift` | Right shift                                      |
| `alt`         | Left or right alt                                |
| `left_alt`    | Left alt                                         |
| `right_alt`   | Right alt                                        |
| `ctrl`        | Left or right control                            |
| `left_ctrl`   | Left control                                     |
| `right_ctrl`  | Right control                                    |
| `gui`         | Left or right GUI key (Windows key, Command key) |
| `left_gui`    | Left GUI key                                     |
| `right_gui`   | Right GUI key                                    |

### Mouse

| Button                | Description                      |
| :-------------------- | :------------------------------- |
| `left_mouse`          | Left mouse button                |
| `middle_mouse`        | Middle mouse button              |
| `right_mouse`         | Right mouse button               |
| `x1_mouse`            | X1 mouse button                  |
| `x2_mouse`            | X2 mouse button                  |
| `wheelup`             | Mouse wheel up                   |
| `wheeldown`           | Mouse wheel down                 |
| `wheelleft`           | Mouse wheel left                 |
| `wheelright`          | Mouse wheel right                |
| `double_left_mouse`   | Double click left mouse button   |
| `double_middle_mouse` | Double click middle mouse button |
| `double_right_mouse`  | Double click right mouse button  |

### Keyboard

| Key                  | Name               |
| :------------------- | :----------------- |
| `a`                  | A                  |
| `b`                  | B                  |
| `c`                  | C                  |
| `d`                  | D                  |
| `e`                  | E                  |
| `f`                  | F                  |
| `g`                  | G                  |
| `h`                  | H                  |
| `i`                  | I                  |
| `j`                  | J                  |
| `k`                  | K                  |
| `l`                  | L                  |
| `m`                  | M                  |
| `n`                  | N                  |
| `o`                  | O                  |
| `p`                  | P                  |
| `q`                  | Q                  |
| `r`                  | R                  |
| `s`                  | S                  |
| `t`                  | T                  |
| `u`                  | U                  |
| `v`                  | V                  |
| `w`                  | W                  |
| `x`                  | X                  |
| `y`                  | Y                  |
| `z`                  | Z                  |
| `1`                  | 1                  |
| `2`                  | 2                  |
| `3`                  | 3                  |
| `4`                  | 4                  |
| `5`                  | 5                  |
| `6`                  | 6                  |
| `7`                  | 7                  |
| `8`                  | 8                  |
| `9`                  | 9                  |
| `0`                  | 0                  |
| `return`             | Return             |
| `escape`             | Escape             |
| `backspace`          | Backspace          |
| `tab`                | Tab                |
| `space`              | Space              |
| `-`                  | -                  |
| `=`                  | =                  |
| `[`                  | [                  |
| `]`                  | ]                  |
| `\`                  | \                  |
| `#`                  | #                  |
| `;`                  | ;                  |
| `'`                  | '                  |
| `,`                  | ,                  |
| `.`                  | .                  |
| `/`                  | /                  |
| `f1`                 | F1                 |
| `f2`                 | F2                 |
| `f3`                 | F3                 |
| `f4`                 | F4                 |
| `f5`                 | F5                 |
| `f6`                 | F6                 |
| `f7`                 | F7                 |
| `f8`                 | F8                 |
| `f9`                 | F9                 |
| `f10`                | F10                |
| `f11`                | F11                |
| `f12`                | F12                |
| `pageup`             | PageUp             |
| `delete`             | Delete             |
| `end`                | End                |
| `pagedown`           | PageDown           |
| `right`              | Right              |
| `left`               | Left               |
| `down`               | Down               |
| `up`                 | Up                 |
| `f13`                | F13                |
| `f14`                | F14                |
| `f15`                | F15                |
| `f16`                | F16                |
| `f17`                | F17                |
| `f18`                | F18                |
| `f19`                | F19                |
| `f20`                | F20                |
| `f21`                | F21                |
| `f22`                | F22                |
| `f23`                | F23                |
| `f24`                | F24                |
