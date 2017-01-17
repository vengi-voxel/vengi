#!/usr/bin/env python3

from optparse import OptionParser

import flext


def main():
    # Read command line arguments and profile settings
    options,profile = parse_args()    
    version,extensions,funcslist,funcsblacklist = flext.parse_profile(profile)

    # Download spec file(s) if necessary
    flext.download_spec(options.download)

    # Parse spec
    passthru, enums, functions, types, raw_enums = flext.parse_xml(version, extensions, funcslist, funcsblacklist)

    # Generate source from templates
    flext.generate_source(options, version, enums, functions, passthru,
                          extensions, types, raw_enums)

    
def parse_args():
    parser = OptionParser(usage='Usage: %prog [options] filename')
    parser.add_option('-d', '--download',
                      action='store_true', dest='download', default=False,
                      help='Force (re-)downloading the spec files before parsing')
    parser.add_option('-D', '--outdir', dest='outdir', default='generated',
                      help='Output directory for generated source files')
    parser.add_option('-T', '--template', dest='template', default='compatible',
                      help='The template set to use for file generation')
    parser.add_option('-t', '--template-dir', dest='template_dir', default=None,
                      help='The directory to look for template files in. (overrides --template)')
    options, args = parser.parse_args()

    if len(args) < 1:
        parser.print_help()
        exit(0)
    elif len(args) > 1:
        parser.print_help()
        exit(1)

    if options.template_dir == None:
        options.template_dir = flext.default_template_root + options.template

    return options, args[0]
    

if __name__ == "__main__":
    main()
