import subprocess
import os

subprocess.run(["git", "init"])
subprocess.call(["git", "submodule", "add", "git@github.com:arctictheory/GameLabs.git"])

print(f"New project generated at: {os.getcwd()}")
