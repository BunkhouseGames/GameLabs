import sys
import pathlib
import subprocess
import json
import click
from utilities import get_unreal_engine

import engine_utils

@click.group()
def cli():
    pass

def _run_commandlet(commandlet, arguments):
    engine = engine_utils.get_engine_root()
    project = get_project_file_path()

    engine_exe = pathlib.Path(engine).joinpath("Engine", "Binaries", "Win64", "UnrealEditor-Cmd.exe")
    cmd = [str(engine_exe), project, f"-run={commandlet}"]
    cmd.extend(arguments)
    print (cmd)

    subprocess.call(cmd)

def get_build_configs():
    build_configs_dict = {}
    # TODO this needs to be made relative to the root 
    build_configs_root = pathlib.Path(__file__).parent.joinpath("buildconfigs")

    print(build_configs_root)
    for file_path in build_configs_root.glob("*.json"):

        with open(str(file_path)) as buildconfig:
            data = json.load(buildconfig)
            build_configs_dict.update(data)

    return build_configs_dict

def get_project_file_path():
    working_directory = pathlib.Path("./").absolute()

    # This might just be the worst code written...
    if working_directory.name == "tools":
        uproject_file_generator = working_directory.parent.glob("*Game/*.uproject")
    else:
        uproject_file_generator = working_directory.glob("*Game/*.uproject")

    uproject_file = list(uproject_file_generator)

    if not len(uproject_file) == 1:
        print("Error: Project File not found or we found too many of them...")
        quit(1)

    uproject_file = uproject_file[0]
    return uproject_file.absolute()

def run_action(profile, should_write_log = False, extra_arguments = ""):
    engine = get_unreal_engine()
    project = get_project_file_path()

    run_uat = pathlib.Path(engine).joinpath("Engine", "Build", "BatchFiles", "RunUAT.bat")

    cmd = [
        str(run_uat),
        "BuildCookRun",
        f"-project={project}",
    ]
    
    if not profile in get_build_configs():
        print(f"Unable to find profile: {profile}")
        
        sys.exit(1)
    
    flags = get_build_configs()[profile]
    cmd.extend(flags)

    if extra_arguments:
        cmd.append(extra_arguments)
    
    print(cmd)

    if should_write_log:
        logfile = open(profile + ".log", 'w')
    
    proc=subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    for line in proc.stdout:
        sys.stdout.write(line.decode('utf-8'))
    
        if should_write_log:
            logfile.write(line.decode('utf-8'))
    
    proc.wait()
    sys.exit(proc.returncode)

@cli.command()
@click.option('--profile', type=str, help='Profile to run')
@click.option('--Log-To-File', is_flag=True, show_default=True, default=False, help='Write Log File')
@click.option('--Extra-Arguments', help='Arguments to append to the command')
def run_build_action(profile, log_to_file, extra_arguments):
    
    if not profile:
        print("Please provide a profile")
        print("Available profiles: \n")

        for each_key in get_build_configs().keys():
            print(each_key)

    else:
        run_action(profile, log_to_file, extra_arguments)

@cli.command()
@click.option('--commandlet', required=True, type=str, help='Commandlet To Run')
@click.option('--Extra-Arguments', type=str, default="", help='Arguments to append to the command')
def run_commandlet(commandlet, extra_arguments):
    extra_arguments = extra_arguments.split(" ")
    _run_commandlet(commandlet, extra_arguments)

if __name__ == '__main__':
    cli()