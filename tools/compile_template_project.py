import sys
import pathlib
import subprocess
import UnrealEnvironment as uEnv

project_file_path = pathlib.Path(sys.argv[1])

ue_path = uEnv.get_unreal_path_from_config("5.0")

def generate_project_files(project_file_path):
    ue_path = uEnv.get_unreal_path_from_config("5.0")

    unreal_build_tool = pathlib.Path(ue_path, "Engine", "Binaries", "DotNET", "UnrealBuildTool", "UnrealBuildTool.exe")
    subprocess.call([unreal_build_tool, "-projectfiles", f"-project={project_file_path}", "-game", "-progress", "-CurrentPlatform", "-NoShippingConfigs"])

generate_project_files(project_file_path)