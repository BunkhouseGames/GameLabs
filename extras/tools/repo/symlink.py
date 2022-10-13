from pathlib import Path
import _winapi

def _find_root(start_path = ""):
    """Searches for the .existence file in the start path directory"""

    if not start_path:
        # By default we use the script path to start the search
        path = Path(__file__).parent
    else:
        path = Path(start_path)
    
    root = ""
    for parent in path.parents:
        match = list(parent.glob(".existence"))
        if len(match) == 1:
            root = parent
            break
        else:
            pass

    if not root:
        print("Error: Unable to find the .existence root in directory tree")
        quit(1)
    else:
        return root
    
def symlink_into_root(source_path):
    print(f"Attempting to map {source_path} into {_find_root()}")

    # _winapi.CreateJunction(source_path, _find_root())


def symlink_from_environment_variable(env_variable, target_folder_name):
    pass

symlink_into_root()