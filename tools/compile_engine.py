import sys
import pathlib
import copy
import subprocess

engine_root = sys.argv[1]
project_file_path = sys.argv[2]

engine = pathlib.Path(engine_root)
project = pathlib.Path(project_file_path)


def compile_engine(engine_root_path, project_path):
    subprocess.run(
        [
            engine_root_path.joinpath("Engine", "Build", "BatchFiles", "RunUBT.bat"),
            "Win64",
            "Development",
            "ShaderCompileWorker",
        ]
    )
    subprocess.run(
        [
            engine_root_path.joinpath("Engine", "Build", "BatchFiles", "RunUBT.bat"),
            "Win64",
            "Development",
            "UnrealPak",
        ]
    )
    subprocess.run(
        [
            engine_root_path.joinpath("Engine", "Build", "BatchFiles", "RunUBT.bat"),
            "Win64",
            "Development",
            "UnrealPak",
            project_path,
            "-TargetType=Editor",
        ]
    )

    pass


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


def build_game(engine, project):
    run_uat = f"{engine}/Engine/Build/BatchFiles/RunUAT.bat"
    cmd = [
        run_uat,
        "BuildCookRun",
        f"-project={project}",
        # f"-archivedirectory={output_folder}",
    ]

    flags = copy.copy(build_flags["kamo_linux_server"])
    cmd.extend(flags)

    subprocess.run(cmd)


compile_engine(engine, project)
build_game(engine, project)
