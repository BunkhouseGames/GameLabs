import sys
import pathlib
import shutil

project_directory = sys.argv[1]

project_directory = pathlib.Path(project_directory)

root_directories_to_remove = ["Binaries", "Intermediate", "DerivedDataCache"]

for root_directory in root_directories_to_remove:
    path_to_delete = project_directory.joinpath(root_directory)
    print(f"Attempting to delete: {path_to_delete}")

    if path_to_delete.exists():
        shutil.rmtree(path_to_delete)


for plugin_directory in project_directory.joinpath(
    "Plugins", "ATCorePlugins"
).iterdir():
    if plugin_directory.is_dir():
        for root_directory in root_directories_to_remove:
            path_to_delete = plugin_directory.joinpath(root_directory)

            if path_to_delete.exists():
                shutil.rmtree(path_to_delete)
