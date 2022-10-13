import subprocess
import pathlib
import os
import shutil

subprocess.run(["git", "init"])

gamelabs_root = pathlib.Path(os.getcwd()).parent.parent
new_project_root = pathlib.Path(os.getcwd())

# print(f"New project generated at: {os.getcwd()}")


def _get_project_name():
    uproject_file = list(new_project_root.glob("**/*.uproject"))[0]
    return uproject_file.stem


name = _get_project_name()

shutil.copytree(
    gamelabs_root.joinpath("extras", "tools"), new_project_root.joinpath("tools")
)


target_path = new_project_root.joinpath(_get_project_name(), "Plugins")
os.makedirs(str(target_path))


shutil.copytree(
    gamelabs_root.joinpath("extras", "ExistenceSDK"),
    new_project_root.joinpath(target_path, "ExistenceSDK"),
)
