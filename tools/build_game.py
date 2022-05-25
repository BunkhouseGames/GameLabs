import sys
import pathlib
import copy
import subprocess

engine_root = sys.argv[1]
project_file_path = sys.argv[2]
profile = sys.argv[3]
output_directory = sys.argv[4]

engine = pathlib.Path(engine_root)
project = pathlib.Path(project_file_path)
output_directory = pathlib.Path(output_directory)

build_flags = {
    "existence_windows_debug_client": [
        "-clientconfig=Debug",
        "-cook",
        "-iterate",
        "-nop4",
        "-build",
        "-stage",
        "-archive",
        "-pak",
        "-nodebuginfo",
        "-noPCH",
        "-prereqs",
        "-CrashReporter",
    ],
    "existence_windows_shipping_client": [
        "-clientconfig=Development",
        "-cook",
        "-iterate",
        "-nop4",
        "-build",
        "-stage",
        "-archive",
        "-nodebuginfo",
        "-noPCH",
        "-prereqs",
        "-CrashReporter",
    ],
    "kamo_linux_server": [
        "-serverconfig=Development",
        "-targetplatform=Linux",
        "-cook",
        "-server",
        "-build",
        "-stage",
        "-archive",
        "-NoClient",
        "-package",
        "-nodebuginfo",
        "-targetplatform=Linux",
        "-prereqs",
    ],
}


def build_game(engine, project, profile, output_directory):
    run_uat = f"{engine}/Engine/Build/BatchFiles/RunUAT.bat"
    cmd = [
        run_uat,
        "BuildCookRun",
        f"-project={project}",
        f"-archivedirectory={output_directory}",
    ]

    flags = copy.copy(build_flags[profile])
    cmd.extend(flags)

    subprocess.run(cmd)


print(f"Engine Path: {engine.absolute()}")

build_game(engine, project, profile, output_directory)
