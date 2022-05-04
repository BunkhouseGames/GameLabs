import subprocess
import os

subprocess.run(["git", "init"])
subprocess.call(["git", "submodule", "add", "git@github.com:arctictheory/GameLabs.git"])

subprocess.call(["git", "submodule", "update", "--init"])
print(f"New project generated at: {os.getcwd()}")
