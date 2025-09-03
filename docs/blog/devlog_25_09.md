# Not much vengi this week

I was quite busy and didn't had much time to work on vengi - but a few things are worth to mention them:

* The object identification and splitting was improved. Previously it wasn't working with hollowed models, as I was just check the 6 face directions - this is now checkout all 26 directions to be able to split hollowed objects properly, too.
* The `resetcamera` command wasn't working in edit mode if you have the viewport option **Apply Transforms** checked. This is fixed now.
* I've tried to improve the `kfa` animation support for `kv6` files. But didn't get that far yet. The slab6 code is really hard to read and follow.

# Other stuff

I've also worked a little bit on World of Padman and prepared a gist [with a script](https://gist.github.com/mgerhardy/5a2d87f0de15f8cfc6932559f195f200) that I am using since some time to cherry-pick commits from ioq3 to wop. This script has the purpose to make cherry-picking possible without conflicts even though one repository has a different formatting-style applied than the other one (World of Padman had a clang-format run, ioq3 does not). Usual cherry-picking can't handle this. The script can be called with `git cherry-pick-format <commit-id>` - what it does is roughly the following:

* check out the parent branch of `<commit-id>`
* create a temp branch
* copy `.clang-format` files from the original working copy into the temp branch working copy
* reformat the files included in `<commit-id>` with clang-format
* commit the changes
* check out `<commit-id>`
* format the patch
* go to temp branch
* copy the changed files from `<commit-id>` over to the temp branch workspace
* create a commit while preserving the original commit meta data (author, date, commit message)
* switch back to the original branch pointer and cherry-pick the newly created and formatted commit
