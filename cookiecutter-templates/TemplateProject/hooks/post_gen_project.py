import subprocess
import os
import shutil
import time

subprocess.call("git init")
subprocess.call(["git", "submodule", "add", "git@github.com:arctictheory/GameLabs.git"])

# os.chdir("..")

print(f"New project generated at: {os.getcwd()}")
