import subprocess
import os

subprocess.call("git init")
subprocess.call(["git", "submodule", "add", "git@github.com:arctictheory/GameLabs.git"])
