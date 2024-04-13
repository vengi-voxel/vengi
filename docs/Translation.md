# Translation

The applications are directly reading `po` files.

> What are `po` files?
> `po` (portable object) files are used by the gettext translation system and a lot of tools exist to edit them (e.g. [poedit](https://poedit.net/))

To create a new translation you should use the `pot` file located in `data/shared` in the repository.

After you've created a new `po` file, copy them into any of the [search paths](Configuration.md) and give it the name as specified [here](https://www.gnu.org/software/gettext/manual/html_node/Locale-Names.html).

> **Example:** Name it after the pattern `ll_CC` where `ll` is an ISO-639 two-letter language code, and `CC` is an ISO-3166 two-letter country code.

If you create a new translation it would be nice if you would contribute it to the project.

## Updating the pot file

There is a `Makefile` target called `pot` - so if you have gnu make installed, just run `make pot` in the root of the project. Otherwise use the tool `xgettext` to extract the strings from the source code.
