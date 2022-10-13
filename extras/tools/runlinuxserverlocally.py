#!/usr/bin/env python3
"""
Make sure the latest game server is running on AWS.
"""
import os
import subprocess
import glob
import sys


PORT = 7777


def run_server():
    # Run Linux server in WSL
    project_name = get_project_name()

    # As a courtesy, notify the user where to find the Kamo DB and how to connect to the server
    local_address = subprocess.check_output("wsl ip addr | grep inet | grep global".split(" ")).strip().decode("ascii")
    # The result is something like:     inet 172.25.21.98/20 brd 172.25.31.255 scope global eth0
    local_address = local_address.split(" ")[1]
    local_address = local_address.split("/")[0]
    print(f"---- LINUX GAME SERVER: {project_name} ------------\n")
    print(f"Connection address: {local_address}:{PORT}")

    linux_user = subprocess.check_output(["wsl", "echo", "$USER"]).strip().decode("ascii")
    print(f"Kamo DB location: \\\\wsl$\\Ubuntu\\home\\{linux_user}\\.kamo")
    print("\n---------------------------------------------------\n")

    # Map the current Windows directory to WSL directory. Assume we are running on Windows
    drive, path = os.path.splitdrive(os.getcwd())  # Give us something like ('D:', '\\work\\ProjectDawn')
    drive = drive[0].lower()
    path_elements = os.path.split(path[1:])  # Note, skip over the leading backslash
    path_elements = "/".join(path_elements)
    executable = f"/mnt/{drive}/{path_elements}/out/LinuxServer/{project_name}Server.sh"

    # Add default and custom args and check for port override
    cmd = ["wsl", executable]
    cmd += sys.argv[1:]
    cmd += ["?listen", "-server", "-NullRHI", "-messaging"]

    custom_args = " ".join(sys.argv[1:])
    if "-port=" not in custom_args:
         cmd.append(f"-port={PORT}")

    print("Executing: " + " ".join(cmd))
    subprocess.run(cmd, shell=True)


def get_project_name():
    """Return the UE project name using the name of the .uproject file."""

    # First look for .uproject file in current directory
    globresult = glob.glob("*.uproject")
    if not globresult:
        # See if the project file is in a subdirectory called <project>Game
        globresult = glob.glob("*Game/*.uproject")
        if not globresult:
            cwd = os.getcwd()
            raise RuntimeError(f"Can't determine project name, *.uproject file not found in {cwd} or in *Game/ subfolder.".format())
    name, ext = os.path.splitext(os.path.split(globresult[0])[1])
    return name


if __name__ == "__main__":
    run_server()




