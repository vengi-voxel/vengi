# How to install data files

Each artifact can get its own directory here - If you depend on an artifact that
installs assets for its own usage, the dependent artifact will also get these
assets installed. Therefore it is important to know, that the directory part of
the original artifact will be removed. An example:

   You have a module names `foo` and it has assets in `data/foo`.
   You also have a module names `bar` which depends on `foo.

   `bar` will now automatically get the files installed that `foo` offers.
   But - not in `$install_prefix/bar/foo/myasset.ext` - but in
   `$install_prefix/bar/myasset.ext`. This is exactly due to the reason that
   you had to specify the `FILES` for the `engine_add_executable` with the full
   path to `foo/myasset.ext`.
