#!/usr/bin/env python3
"""
Make sure the latest game server is running on AWS.
"""
import sys
import os
import re
import subprocess


def ensure_latest():
    # Simply call kamo commands to run the latest game server

    versionfile = os.path.abspath("VERSION")

    if not os.path.exists(versionfile):
        raise RuntimeError(f"Can't find version file {versionfile}")

    with open(versionfile) as f:
        semantic_version = f.read().strip()

    print(f"Version \"{semantic_version}\" found in version file {versionfile}")
    subprocess.run("kamo nodes ensure 1", shell=True)

    cmd = f"kamo jobs create -y -v {semantic_version} " + " ".join(sys.argv[1:])
    print(f"Execute kamo command: {cmd}")
    subprocess.run(cmd, shell=True)



if __name__ == "__main__":
    ensure_latest()




