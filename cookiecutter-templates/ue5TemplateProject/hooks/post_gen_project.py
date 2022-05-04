import subprocess
import os

subprocess.run(["git", "init"])
subprocess.call(["add-gamelabs-submodules.bat"])

subprocess.call(["git", "submodule", "update", "--init"])
print(f"New project generated at: {os.getcwd()}")
