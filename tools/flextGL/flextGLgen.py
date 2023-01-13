#!/usr/bin/env python3

import re
import sys
import os.path
from optparse import OptionParser
import xml.etree.ElementTree as etree
import urllib.parse

import flext

_gl_angle_ext_patch_re = re.compile(r'<ptype>\s*(const\s+)?\s*GL([a-z0-9]+)(\s*\*+)?\s*</ptype>')
_gl_angle_ext_patch_sub = r'\1<ptype>GL\2</ptype>\3'

_gl_angle_ext_patch_void_re = re.compile(r'<ptype>\s*(const\s+)?void(\s*\*+)?\s*</ptype>')
_gl_angle_ext_patch_void_sub = r'\1void\2'

def main(argsstring, options, profile):
    version, extraspecs, extensions, funcslist, funcsblacklist = flext.parse_profile(profile)

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
            spec_url = flext.vk_spec_url.format('main')
            spec_file = 'vk.xml'
    else:
        spec_url = flext.gl_spec_url
        spec_file = 'gl.xml'

    flext.download_spec(spec_url, spec_file, options.download)
    xml = etree.parse(os.path.join(flext.spec_dir, spec_file))
    root = xml.getroot()

    # If there are extra spec files, download them and merge into the main one
    for extraspec in extraspecs:
        # If extraspec exists locally, use directly, otherwise download
        if os.path.exists(os.path.join(os.path.dirname(profile), extraspec)):
            spec_file_local = os.path.join(os.path.dirname(profile), extraspec)
        else:
            spec_file = os.path.basename(urllib.parse.urlparse(extraspec).path)
            spec_file_local = os.path.join(flext.spec_dir, spec_file)
            flext.download_spec(extraspec, spec_file, options.download)

        # IF ONLY it would be so easy, implementing a generic way to merge the
        # spec files, eh? NOPE, WE'RE GOOGLE, WE DON'T CARE TO PUBLISH THE
        # EXTENSIONS UPSTREAM AND WE DON'T CARE ABOUT HAVING THE FILE VALID
        # EITHER! CHROME! FTW!! WE OWN THE WORLD, YAKNOW?!?!
        with open(spec_file_local, 'r') as f:
            data = f.read()
            # 1. gl_angle_ext.xml uses `<ptype>const GLtype *</ptype>` instead
            #    of `const <ptype>GLsizei</ptype> *` so we have to patch it
            #   first to get something valid
            data = re.sub(_gl_angle_ext_patch_re, _gl_angle_ext_patch_sub, data)
            # 2. `<ptype>const void*</ptype>` is not valid either, it should
            #    not be wrapped in that tag at all
            data = re.sub(_gl_angle_ext_patch_void_re, _gl_angle_ext_patch_void_sub, data)

        # ET, THE FUCK is this?! https://stackoverflow.com/a/18281386
        extra = etree.ElementTree(etree.fromstring(data)).getroot()

        # The <comment> field is ignored

        # The <extensions> and <commands> contents are merged
        for i in ['extensions', 'commands']:
            extension_point = root.findall('./{}'.format(i))[0];
            for member in extra.iter(i):
                extension_point.extend(member)

        # The <enums> blocks are appended. Ideally this would be inserted right
        # after all the <enums> in the original file, but it doesn't really
        # matter FOR THE MACHINE GOD so I won'ŧ bother.
        for enum in extra.iter('enums'):
            root.append(enum)

    # Might be useful for debugging someday
    # xml.write(os.path.join(flext.spec_dir, 'merged_' + spec_file))

    # Parse spec
    passthru, enums, functions, types, raw_enums = flext.parse_xml(root, version, extensions, funcslist, funcsblacklist)

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
