#!/usr/bin/env python3

import re
import sys
import textwrap
from dateutil.parser import parse
from datetime import datetime, timezone

def parse_markdown_changelog(markdown_changelog: str) -> str:
    # Use a regular expression to match the changelog entries in the Markdown document
    changelog_regex = r"^##\s+(\d+\.\d+\.\d+)\s+\((\d{4}-..-..)\)\n((?:[^##])+)$"
    cleanup_change_regex = r"^-\s+"
    url_regex = r"\[.+\]\((.+)\)"
    changelog_entries = re.findall(changelog_regex, markdown_changelog, flags=re.MULTILINE)
    package_name = "vengi"
    output = ""

    for entry in changelog_entries:
        version, date, sections = entry
        release = "unstable"
        parsed_date = None
        try:
            parsed_date = parse(date + " 00:00:00-00:00")
        except:
            release = "UNRELEASED"
            parsed_date = datetime.now(timezone.utc)
        formatted_date = parsed_date.strftime("%a, %-d %b %Y %H:%M:%S %z")

        firstchange = True
        output += f"{package_name} ({version}.0-1) {release}; urgency=low\n\n"
        for section in sections.strip().split("\n\n"):
            changes = section.strip().split("\n")
            for change in changes:
                change = re.sub(cleanup_change_regex, "", change.strip())
                if not firstchange:
                    if change.endswith(":"):
                        output += "\n"
                firstchange = False
                change = change.replace("`", "")
                change = re.sub(url_regex, r"\1", change)
                wrapped_lines = textwrap.wrap(change, width=79, initial_indent="  * ", subsequent_indent="    ")
                for line in wrapped_lines:
                    output += f"{line}\n"
        output += f"\n -- Martin Gerhardy <martin.gerhardy@gmail.com>  {formatted_date}\n\n"

    return output

if __name__ == "__main__":
    input_file = sys.argv[1]
    # Read the Markdown changelog from the input file
    with open(input_file, "r") as f:
        markdown_changelog = f.read()

    print(parse_markdown_changelog(markdown_changelog))
