#!/usr/bin/env python3

import re
import sys
import xml.etree.ElementTree as ET
from datetime import datetime

MIN_DATE = datetime(2024, 1, 1)

def parse_markdown_versions(markdown_changelog: str):
    changelog_regex = r"^##\s+(\d+\.\d+\.\d+)\s+\((\d{4}-\d{2}-\d{2})\)"
    entries = re.findall(changelog_regex, markdown_changelog, flags=re.MULTILINE)
    results = []

    for version, date_str in entries:
        try:
            parsed_date = datetime.fromisoformat(date_str)
            if parsed_date >= MIN_DATE:
                results.append(("v" + version, date_str))
        except Exception as e:
            print(f"Skipping invalid date '{date_str}' for version {version}: {e}")
            continue
    return results

def get_existing_versions(xml_path):
    tree = ET.parse(xml_path)
    root = tree.getroot()
    releases_elem = root.find("releases")
    versions = [r.attrib["version"] for r in releases_elem.findall("release")]
    return versions, tree, releases_elem, root

def add_missing_versions_to_xml(missing_versions, releases_elem):
    for version, date in reversed(missing_versions):  # newest entries first
        release_elem = ET.Element("release", {
            "version": version,
            "date": date,
            "type": "stable"
        })
        url_elem = ET.SubElement(release_elem, "url")
        url_elem.text = f"https://github.com/vengi-voxel/vengi/releases/tag/{version}"
        releases_elem.insert(0, release_elem)

def indent(elem, level=0):
    i = "\n" + level * "  "
    if len(elem):
        if not elem.text or not elem.text.strip():
            elem.text = i + "  "
        for e in elem:
            indent(e, level + 1)
        if not e.tail or not e.tail.strip():
            e.tail = i
    if level and (not elem.tail or not elem.tail.strip()):
        elem.tail = i
    return elem

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: metainfo.py <appdata.xml> <changelog.md>")
        sys.exit(1)

    xml_path = sys.argv[1]
    changelog_path = sys.argv[2]

    # Read changelog
    with open(changelog_path, "r") as f:
        markdown = f.read()

    changelog_versions = parse_markdown_versions(markdown)
    xml_versions, tree, releases_elem, root = get_existing_versions(xml_path)

    # Determine missing versions (from 2024+)
    missing = [entry for entry in changelog_versions if entry[0] not in xml_versions]

    if not missing:
        print("No new versions to add.")
        sys.exit(0)

    add_missing_versions_to_xml(missing, releases_elem)
    indent(root)
    tree.write(xml_path, encoding="utf-8", xml_declaration=True)
    print(f"Added {len(missing)} release(s) from 2024+ to {xml_path}")
