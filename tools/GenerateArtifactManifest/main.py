import sys
import pathlib
import subprocess
import datetime
import json
import os

name = sys.argv[1]
repo_root = sys.argv[2]
project_root = sys.argv[3]
output_directory = sys.argv[4]

repo_root_path = pathlib.Path(repo_root)
output_file_path = pathlib.Path(output_directory).joinpath("buildinfo.json")

# BUILD_MANIFEST_FILE_NAME = "buildinfo.json"
LINUX_SERVER_EXECUTABLE = "LinuxServer/KamoOneProject/Binaries/Linux/KamoOneProjectServer"

def get_sha():
    ret = subprocess.check_output(
        ["git", "rev-parse", "--short", "HEAD"], universal_newlines=True
    ).strip()
    return ret

def get_branch():
    return subprocess.check_output(["git", "rev-parse", "--abbrev-ref", "HEAD"], universal_newlines=True).strip()

def get_version():
    return "Dawn_" + str(datetime.datetime.utcnow().isoformat()) +"_"+ get_sha() 

def get_user_name():
    return "Not-Set"

def write_manifest(repo_root, project_root, manifest_file, name):

    manifest = {
        "name": name,
        "commit": get_sha(),
        "branch": get_branch(),
        "version": get_version(),
        "build_number": os.environ.get("BUILD_NUMBER", "0"),
        "timestamp": datetime.datetime.utcnow().isoformat(),
        "user": get_user_name(),
        "executable": LINUX_SERVER_EXECUTABLE,
    }
    print(f"Generating manifest for: {name} into {manifest_file}")

    with open(str(manifest_file), "w") as json_file:
        json.dump(manifest, json_file, indent=4)


write_manifest(repo_root_path, project_root, output_file_path, name)