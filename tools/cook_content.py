import sys
import pathlib
import copy
import subprocess

engine_root = sys.argv[1]
project_file_path = sys.argv[2]

engine = pathlib.Path(engine_root)
project = pathlib.Path(project_file_path)


cmd_flags = {
    "cook_windows_server": [
        "-targetplatform=Win64",
        "-stdout",
        "-unattended",
        "-cook",
        "-iterate",
        "-nodebuginfo",
        "-noPCH",
        "-nocompile",
    ],
}


def build_game(engine, project):
    run_uat = f"{engine}/Engine/Build/BatchFiles/RunUAT.bat"

    cmd = [
        run_uat,
        "BuildCookRun",
        f"-project={project}",
    ]

    flags = copy.copy(cmd_flags["cook_windows_server"])
    cmd.extend(flags)

    subprocess.run(cmd)


print(f"Engine Path: {engine.absolute()}")

build_game(
    engine,
    project,
)
