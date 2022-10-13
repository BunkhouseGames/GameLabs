#!/usr/bin/env python3
"""
Generate manifest file for game server.

This is a json file which contains a version and a path to the executable file.
"""
import sys
import os
import json
import glob
import datetime
import getpass
import subprocess
import pathlib

def get_commit_sha():
    return subprocess.check_output(["git", "rev-parse", "--short", "HEAD"], universal_newlines=True).strip()


def get_branch():
    return subprocess.check_output(["git", "rev-parse", "--abbrev-ref", "HEAD"], universal_newlines=True).strip()


def get_project_name():
    """Return the UE project name using the name of the .uproject file."""

    # First look for .uproject file in current directory
    globresult = glob.glob("*.uproject")
    if not globresult:
        # See if the project file is in a subdirectory called <project>Game
        globresult = glob.glob("*Game/*.uproject")
        if not globresult:
            cwd = os.getcwd()
            raise RuntimeError(f"Can't determine project name, *.uproject file not found in {cwd} or in *Game/ subfolder.".format())
    name, ext = os.path.splitext(os.path.split(globresult[0])[1])
    return name

def get_default_game_config_file_path():
    uproject_file = glob.glob("*Game/*.uproject")[0]
    default_game_config = pathlib.Path(uproject_file).parent.joinpath("Config", "DefaultGame.ini")
    return default_game_config.absolute()

def get_version_file():
    return os.path.abspath("VERSION")

def update_version_in_project():
    default_game_config = get_default_game_config_file_path()

    if default_game_config.exists():
        f = open(default_game_config, "r")
        lines = f.readlines()
        f.close()
        
        new_lines = []
        for each_line in lines:
            if each_line.startswith("ProjectVersion="):
                each_line = "ProjectVersion=" + get_version() + "\n"

            new_lines.append(each_line)

        f = open(default_game_config, "w")
        f.writelines(new_lines)
        f.close()

def get_version():
    versionfile = get_version_file()
    if not os.path.exists(versionfile):
        raise RuntimeError(f"Can't determine version, {versionfile} not found.".format())
    with open(versionfile) as f:
        return f.read().strip()


def generate_manifest():
    """Returns a manifest dict for a Linux server"""
    project_name = get_project_name()
    commit_sha = get_commit_sha()

    manifest = {
        "name": project_name,
        "commit": commit_sha,
        "branch": get_branch(),
        "version": get_version(),
        "timestamp": datetime.datetime.utcnow().isoformat(),
        "user": getpass.getuser(),
        "executable": f"{project_name}/Binaries/Linux/{project_name}Server".format()
    }

    return manifest

def push_version_update_to_git():
    
    files_to_add = [
        get_version_file(),
        str(get_default_game_config_file_path())
    ]

    for file_to_add in files_to_add:
        cmd = ["git", "add", file_to_add]
        print(f"Running: {cmd}")
        subprocess.call(cmd)

    commit_cmd = ["git", "commit", "-m", f"Automatically updating to version {get_version()}"]
    print(f"Running command: {commit_cmd}")

    subprocess.call(commit_cmd)

    push_cmd = ["git", "push"]
    print(f"Running command: {push_cmd}")
    subprocess.call(push_cmd)

if __name__ == "__main__":
    manifest = generate_manifest()
    manifest_json = json.dumps(manifest, indent=4)
    manifest_filename = os.path.abspath("ProjectDawnGame/Saved/StagedBuilds/LinuxServer/buildinfo.json")
    print(f"Manifest:\n\n{manifest_json}\n")
    print(f"Writing manifest file {manifest_filename}")
    with open(manifest_filename, "w") as f:
        f.write(manifest_json)

    update_version_in_project()
    push_version_update_to_git()




