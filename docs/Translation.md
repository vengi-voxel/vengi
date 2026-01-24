# Translation

The applications are directly reading `po` files.

> What are `po` files?
> `po` (portable object) files are used by the gettext translation system and a lot of tools exist to edit them (e.g. [poedit](https://poedit.net/))

To create a new translation you should use the `pot` file located in `data/shared` in the repository.

After you've created a new `po` file, copy them into any of the [search paths](Configuration.md) (either directly, or in a folder named `po`) and give it the name as specified [here](https://www.gnu.org/software/gettext/manual/html_node/Locale-Names.html).

> **Example:** Name it after the pattern `ll_CC` where `ll` is an ISO-639 two-letter language code, and `CC` is an ISO-3166 two-letter country code.

If you create a new translation it would be nice if you would contribute it to the project.

## Missing glyphs

If your translation uses unicode character which are not included in the default (Arimo) font, you can register additional fonts by putting them into a subdirectory called `font`. E.g. `NotoSansSC-Regular.ttf` for chinese glyphs. Put it into `voxedit/font/NotoSansSC-Regular.ttf` (if the directory doesn't exist, just create it in any of the [search paths][Configuration.md]).

## Developers

> Translators don't have to do this

### Updating the pot file

After new string were added in the code, you have to update the `pot` file to make those strings available to the translators.

There is a `Makefile` target called `pot` - so if you have gnu make installed, just run `make pot` in the root of the project. Otherwise use the tool `xgettext` to extract the strings from the source code.

### Mark strings as being translatable

use the `_` macro to mark a string as being translatable. E.g. `_("my string")`

For only extracting them, but not translating them, you can use `N_("my string")`. Now `my string` appears in the `pot` file, but the location where the string is stored, is still the english string `my string` - not anything translated.

By adding a context to the translators you could make things clearer, e.g. use `C_("Some string that describes the translatable string", "my string")` to add a `msgctxt` line to the pot file. And there is also a `NC_` version that works like the above mentioned `N_`.
