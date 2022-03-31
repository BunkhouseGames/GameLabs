import sys
import pathlib
import subprocess
input_folder_path = sys.argv[1]

path = pathlib.Path(input_folder_path)
# subprocess.run([path.joinpath("setup.bat"), "--force"])
subprocess.run([path.joinpath("Engine", "Build", "BatchFiles", "GenerateProjectFiles.bat")])
subprocess.run([path.joinpath("Engine", "Build", "BatchFiles", "RunUBT.bat"), "Win64", "Development", "UnrealEditor"])
