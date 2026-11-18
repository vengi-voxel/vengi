#!/usr/bin/env python3

import os
import re
import xml.etree.ElementTree as ET

def get_script_dir():
    return os.path.dirname(os.path.realpath(__file__))

def thumbnailer_excludes(xml_file):
    text = open(xml_file, encoding="utf-8").read()
    match = re.search(r"thumbnailer-exclude:\s*(.*?)\s*-->", text)
    if not match:
        return set()
    return {item.strip() for item in match.group(1).split(";") if item.strip()}

def extract_mimetypes(xml_file, exclude=None):
    ns = {'x': 'http://www.freedesktop.org/standards/shared-mime-info'}
    tree = ET.parse(xml_file)
    root = tree.getroot()
    mimetypes = sorted(set(elem.attrib['type'] for elem in root.findall(".//x:mime-type", ns)))
    if exclude:
        mimetypes = [mime for mime in mimetypes if mime not in exclude]
    return ';'.join(mimetypes)

def replace_mimetype_line(file_path, mimetypes):
    tmp_file = file_path + ".tmp"
    with open(file_path, "r") as infile, open(tmp_file, "w") as outfile:
        for line in infile:
            if line.startswith("MimeType="):
                outfile.write(f"MimeType={mimetypes}\n")
            else:
                outfile.write(line)
    os.replace(tmp_file, file_path)

def main():
    base_dir = get_script_dir()
    os.chdir(base_dir)

    mime_xml = "x-voxel.xml"
    desktop_file = "voxedit.desktop.in"
    thumbnailer_file = "thumbnailer.thumbnailer.in"

    mimetypes = extract_mimetypes(mime_xml)
    thumbnailer_mimetypes = extract_mimetypes(mime_xml, thumbnailer_excludes(mime_xml))

    replace_mimetype_line(desktop_file, mimetypes)
    replace_mimetype_line(thumbnailer_file, thumbnailer_mimetypes)

    print(f"Updated MimeType lines in {desktop_file} and {thumbnailer_file}")

if __name__ == "__main__":
    main()
