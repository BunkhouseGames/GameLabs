import os 
import copy
import pathlib
import subprocess

build_flags = {
    "existence_windows_debug_client": [
        "-clientconfig=Debug",
        "-cook",
        "-iterate",
        "-nop4",
        "-nocompileeditor",
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
        "-nocompileeditor",
        "-build",
        "-stage",
        "-archive",
        "-pak",
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
        "-nocompileeditor",
        "-build",
        "-stage",
        "-archive",
        "-pak",
        "-NoClient",
        "-package",
        "-nodebuginfo",
        "-targetplatform=Linux",
        "-prereqs",
    ],
}

engine_path = pathlib.Path("D:/Work/Epic/UE_5.0")
project_path = pathlib.Path("D:/Work/ArcticTheory/ProjectDawn/ProjectDawnGame/ProjectDawnPreview.uproject")

run_uat = f"{engine_path}/Engine/Build/BatchFiles/RunUAT.bat"
cmd = [run_uat,
        "BuildCookRun",
        f"-project={project_path}",
        # f"-archivedirectory={output_folder}",
    ]

flags = copy.copy(build_flags["existence_windows_shipping_client"])
cmd.extend(flags)

subprocess.run(cmd)