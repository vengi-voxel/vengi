#!/usr/bin/env python3

import sys
import os.path
from optparse import OptionParser

import flext


def main(argsstring, options, profile):
    version,extensions,funcslist,funcsblacklist = flext.parse_profile(profile)

    # Download spec file(s) if necessary
    if version.api == 'vulkan':
        if version.release:
            version_string = 'v{}.{}.{}'.format(version.major, version.minor, version.release)
            if version.major == 1 and version.minor == 0:
                spec_url = flext.vk_spec_url10.format(version_string)
            else:
                spec_url = flext.vk_spec_url.format(version_string)
            spec_file = 'vk.{}.xml'.format(version_string)
        else:
            spec_url = flext.vk_spec_url.format('master')
            spec_file = 'vk.xml'
    else:
        spec_url = flext.gl_spec_url
        spec_file = 'gl.xml'

    flext.download_spec(spec_url, spec_file, options.download)

    # Parse spec
    passthru, enums, functions, types, raw_enums = flext.parse_xml(spec_file, version, extensions, funcslist, funcsblacklist)

    # Generate source from templates
    flext.generate_source(argsstring, options, version, enums, functions,
        passthru, extensions, types, raw_enums)


def parse_args(): # pragma: no cover
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
        options.template_dir = os.path.join(flext.default_template_root, options.template)

    return options, args[0]


if __name__ == "__main__": # pragma: no cover
    # Read command line arguments and profile settings
    main(' '.join(sys.argv[1:]), *parse_args())
