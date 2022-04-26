import sys
import pathlib
import zipfile
import subprocess

engine_root = sys.argv[1]
project_file_path = sys.argv[2]


def compile_engine(engine_root_path, project_path):
    subprocess.run([engine_root_path.joinpath("Engine", "Build", "BatchFiles", "RunUBT.bat"), "Win64", "Development", "ShaderCompileWorker"])
    subprocess.run([engine_root_path.joinpath("Engine", "Build", "BatchFiles", "RunUBT.bat"), "Win64", "Development", "UnrealPak"])
    subprocess.run([engine_root_path.joinpath("Engine", "Build", "BatchFiles", "RunUBT.bat"), "Win64", "Development", "UnrealPak", "D:/Work/ArcticTheory/ProjectDawn/ProjectDawnGame/ProjectDawnPreview.uproject", "-TargetType=Editor"])

    pass

engine  = pathlib.Path(engine_root)
project = pathlib.Path(project_file_path)

print(engine)
print(project)

compile_engine(engine, project)