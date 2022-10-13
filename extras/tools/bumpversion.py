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


def bump_version(version_part=None, include_git_hash=True, generate_release_notes=True):
    version_part = version_part or "patch"
    versionfile = os.path.abspath("VERSION")

    parts = ["major", "minor", "patch"]
    if version_part not in parts:
        raise RuntimeError(f"Version part must be one of {parts}")

    if not os.path.exists(versionfile):
        raise RuntimeError(f"Can't find version file {versionfile}")

    with open(versionfile) as f:
        semantic_version = f.read().strip()

    print(f"Version \"{semantic_version}\" found in version file {versionfile}")

    search_result = re.search(PATTERN, semantic_version)
    if not search_result:
        raise RuntimeError(f"Not a semantic version: {semantic_version}. See https://semver.org/")

    # The dict is something like: {'major': '1', 'minor': '0', 'patch': '1', 'prerelease': None, 'buildmetadata': None}
    version = search_result.groupdict()
    print(f"Parsed {semantic_version} into {version}")

    # Update build metadata
    if include_git_hash:
        version["buildmetadata"] = get_commit_sha()

    # Increase version
    version[version_part] = str(int(version[version_part]) + 1)

    # Reset minor and/or patch verions if needed
    if version_part == "major":
        version["minor"] = 0
        version["patch"] = 0
    elif version_part == "minor":
        version["patch"] = 0

    new_version = "{major}.{minor}.{patch}".format(**version)
    if version["prerelease"]:
        new_version += "-{prerelease}".format(**version)
    if version["buildmetadata"]:
        new_version += "+{buildmetadata}".format(**version)

    print(f"New version: {new_version}")

    with open(versionfile, "w") as f:
        f.write(new_version)

    if generate_release_notes:
        # Get commit log since VERSION file was last modififed
        cmd = "git log -n 1 --pretty=format:%H VERSION"
        version_file_commit_hash = subprocess.check_output(cmd, universal_newlines=True).strip()
        version_file_commit_hash = "015de09bfbdc"
        #print(f"verison file com mit hash: {version_file_commit_hash}")
        cmd = f'git log {version_file_commit_hash}..HEAD --pretty=format:"%h %as%d %s <%an>" --abbrev-commit'
        print(f"Generating release notes report using: {cmd}")
        release_notes = subprocess.check_output(cmd, universal_newlines=True).strip()
        # Prune the release notes a little bit
        release_notes = release_notes.replace("(HEAD -> master, origin/master, origin/HEAD) ", "")  # Strip this out
        lines = [line for line in release_notes.splitlines() if "Merge branch" not in line and "Automatically updating to version" not in line]
        release_notes = "\n".join(lines)
        print(f"releas notes:\n{release_notes}")
        pretty_version = semantic_version.split('+')[0]  # Strip any meta info from version
        with open(f"release-notes-{pretty_version}.txt", "w") as f:
            utcnow = str(datetime.utcnow()).split('.')[0]
            f.write(f"Release {pretty_version} built at {utcnow} UTC\n\n{release_notes}\n")


def get_commit_sha():
    return subprocess.check_output(["git", "rev-parse", "--short", "HEAD"], universal_newlines=True).strip()

if __name__ == "__main__":

    version_part = None
    if len(sys.argv) > 1:
        version_part = sys.argv[1]
    bump_version(version_part)
