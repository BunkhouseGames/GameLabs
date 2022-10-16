import pathlib
import sys
import os

def find_mono_repo_root(search_path = ""):
    """
    Walk down up the folder structure to find the .existence file to determine the root 
    of the project
    """
    if not search_path:
        search_path = pathlib.Path(os.getcwd())
    else:
        search_path = pathlib.Path(search_path)

    if search_path.parent == search_path:
        print("Error: Reached the bottom and nothing was found")
        sys.exit(1)

    files = list(search_path.glob(".existence"))

    if len(files) != 1:
        # The .existence file is not found so we go up one level
        return find_mono_repo_root(search_path.parent)

    else:
        mono_repo_root = files[0].parent
        return mono_repo_root
  
def get_unreal_engine():
    """
    Return the path to the configured unreal engine
    """
    root = find_mono_repo_root()
    symlinked_unreal_engine_path = root.joinpath("UnrealEngine")
    resolved_symlink_path = os.readlink(symlinked_unreal_engine_path)
    
    # TODO 
    # I'm trying to resolve the symlink path so that I can use it directly (there are a few cases where we can't just directly
    # use the symlinked path...) I'm manually stripping away this \\? from the front of the string ... which I'm pretty sure is wrong
    engine_path = resolved_symlink_path[4:]
    unreal_engine_path = pathlib.Path(engine_path)

    if not unreal_engine_path.exists():
        print("Error: Unreal Engine Path not found in the repo root")
        sys.exit(1)

    return pathlib.Path(engine_path)