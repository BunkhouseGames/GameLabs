import subprocess
import pathlib
import os
import glob
import sys


def find_project():
    root = pathlib.Path(__file__).parent.absolute()

    for item in root.iterdir():
        if item.is_dir():

            for subfile in pathlib.Path(item).iterdir():

                if subfile.suffix == ".uproject":
                    return subfile.parent


def install_at_gamelabs_modules():

    project_root = find_project()
    if not project_root:
        print("error: no project directory found")
        sys.exit(4)

    os.chdir(project_root)

    subprocess.run("git init")

    core_plugins = [
        ["git@github.com:arctictheory/kamo.git", "Kamo"],
        ["git@github.com:arctictheory/RedisClient.git", "RedisClient"],
        ["git@github.com:arctictheory/ATTools.git", "ATTools"],
        ["git@github.com:arctictheory/SideQuest.git", "SideQuest"],
        ["git@github.com:arctictheory/ATCommon.git", "ATCommon"],
    ]

    for url, name in core_plugins:
        cmd = ["git", "submodule", "add", url, f"Plugins/ATCorePlugins/{name}"]
        subprocess.run(cmd)


install_at_gamelabs_modules()
