import sys
import pathlib
import copy
import subprocess

engine_root = sys.argv[1]
project_file_path = sys.argv[2]
profile = sys.argv[3]

engine = pathlib.Path(engine_root).absolute()
project = pathlib.Path(project_file_path).absolute()

print(f"Engine Root: {engine}")
print(f"Project File Path: {project}")
print(f"Profile: {profile}")

build_flags = {
    "existence_windows_debug_client": [
        "-clientconfig=Debug",
        "-cook",
        "-iterate",
        "-nop4",
        "-build",
        "-stage",
        "-pak",
        "-nodebuginfo",
        "-noPCH",
        "-prereqs",
        "-iostore",
        "-CrashReporter",
    ],
    "existence_windows_shipping_client": [
        "-clientconfig=Development",
        "-cook",
        "-iterate",
        "-nop4",
        "-build",
        "-stage",
        "-iostore",
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
        "-iostore",
        "-NoClient",
        "-package",
        # "-nodebuginfo",
        "-targetplatform=Linux",
        "-prereqs",
    ],
}


def build_game(engine, project, profile):
    
    run_uat = pathlib.Path(engine).joinpath("Engine", "Build", "BatchFiles", "RunUAT.bat")

    cmd = [
        str(run_uat),
        "BuildCookRun",
        f"-project={project}",
    ]

    flags = copy.copy(build_flags[profile])
    cmd.extend(flags)

    print(f"running command: " + " ".join(cmd))
    ret = subprocess.run(cmd)
    
    # exiting with the correct exit code
    sys.exit(ret.returncode)


print(f"Engine Path: {engine.absolute()}")

build_game(engine, project, profile)
