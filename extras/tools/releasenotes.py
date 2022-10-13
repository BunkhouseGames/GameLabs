#!/usr/bin/env python3
"""
Bump version number in a semantic version file VERSION.
"""
import sys
import os
import re
import subprocess
from datetime import datetime


# Using semantic versioning as described in https://semver.org/

# The regex pattern from server.org and found in https://regex101.com/r/Ly7O1x/3/
PATTERN = r"^(?P<major>0|[1-9]\d*)\.(?P<minor>0|[1-9]\d*)\.(?P<patch>0|[1-9]\d*)(?:-(?P<prerelease>(?:0|[1-9]\d*|\d*[a-zA-Z-][0-9a-zA-Z-]*)(?:\.(?:0|[1-9]\d*|\d*[a-zA-Z-][0-9a-zA-Z-]*))*))?(?:\+(?P<buildmetadata>[0-9a-zA-Z-]+(?:\.[0-9a-zA-Z-]+)*))?$"


def generate_release_notes():

    versionfile = os.path.abspath("VERSION")
    if not os.path.exists(versionfile):
        raise RuntimeError(f"Can't find version file {versionfile}")

    with open(versionfile) as f:
        semantic_version = f.read().strip()

    print(f"Version \"{semantic_version}\" found in version file {versionfile}")

    # Get commit log since VERSION file was last modififed
    cmd = "git log -n 1 --pretty=format:%H VERSION"
    version_file_commit_hash = subprocess.check_output(cmd, universal_newlines=True).strip()
    ##version_file_commit_hash = "015de09bfbdc"

    cmd = f'git log {version_file_commit_hash}..HEAD --pretty=format:"%h %as%d %s <%an>" --abbrev-commit'
    print(f"Generating release notes report using: {cmd}\n")
    release_notes = subprocess.check_output(cmd, universal_newlines=True).strip()

    # Prune the release notes a little bit
    release_notes = release_notes.replace("(HEAD -> master, origin/master, origin/HEAD) ", "")  # Strip this out
    lines = [line for line in release_notes.splitlines() if "Merge branch" not in line and "Automatically updating to version" not in line]
    release_notes = "\n".join(lines)

    pretty_version = semantic_version.split('+')[0]  # Strip any meta info from version
    utcnow = str(datetime.utcnow()).split('.')[0]
    release_notes_text = f"Release {pretty_version} built at {utcnow} UTC\n\n{release_notes}\n"
    print(release_notes_text)

    with open(f"release-notes-{pretty_version}.txt", "w") as f:
        f.write(release_notes_text)


if __name__ == "__main__":
    generate_release_notes()
