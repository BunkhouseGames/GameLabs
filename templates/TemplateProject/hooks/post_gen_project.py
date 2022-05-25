import subprocess
import os
import shutil


subprocess.call("git init")

# Clone myself into the target project
subprocess.call(["git", "submodule", "add", "git@github.com:arctictheory/GameLabs.git"])

# Setup AT Game labs
core_plugins = [
    ["git@github.com:arctictheory/kamo.git", "Kamo"],
    ["git@github.com:arctictheory/RedisClient.git", "RedisClient"],
    ["git@github.com:arctictheory/ATTools.git", "ATTools"],
    ["git@github.com:arctictheory/SideQuest.git", "SideQuest"],
    ["git@github.com:arctictheory/ATCommon.git", "ATCommon"],
    ["git@github.com:arctictheory/LandmassEngine.git", "GenOS"],
    ["git@github.com:arctictheory/ATUpVote.git", "ATUpVote"],
]

for url, name in core_plugins:
    cmd = ["git", "submodule", "add", url, f"Plugins/ATCorePlugins/{name}"]
    subprocess.run(cmd)

print(f"New project generated at: {os.getcwd()}")
