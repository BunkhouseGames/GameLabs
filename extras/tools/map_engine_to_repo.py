import winreg
import _winapi
import sys
import os
import pathlib
import click

@click.group()
def cli():
    pass

def make_symlink_from_custom_path(ue_target_in_repo, path_to_ue):

    # Remove the old junction if it exists
    if pathlib.Path(ue_target_in_repo).exists():
        os.rmdir(ue_target_in_repo)

    src = str(path_to_ue)
    target = str(ue_target_in_repo)

    click.echo(f"Mapped: {src} to {target}")
    _winapi.CreateJunction(src, target)

def get_unreal_path_from_registry(version):
    try:
        registry_path = f"SOFTWARE\\EpicGames\\Unreal Engine\\{version}"
        name = "InstalledDirectory"

        registry_key = winreg.OpenKey(
            winreg.HKEY_LOCAL_MACHINE, registry_path, 0, winreg.KEY_READ
        )
        value, regtype = winreg.QueryValueEx(registry_key, name)
        winreg.CloseKey(registry_key)

        return value

    except WindowsError:
        print(f"Error: No Unreal Engine {version} found in registry")
        raise

@cli.command()
@click.option('--root_directory', required=True, type=str, help='Root path for the repo')
@click.option('--engine_path', required=True, type=str, help='Unreal Engine version')

def map_source_engine(root_directory, engine_path):
    """Maps Source Engine to Repo"""
    root_directory = pathlib.Path(root_directory).resolve()
    
    ue_from_registry = pathlib.Path(engine_path)
    ue_target_in_repo = pathlib.Path(root_directory).joinpath("UnrealEngine")

    make_symlink_from_custom_path(ue_target_in_repo, ue_from_registry)

@cli.command()
@click.option('--root_directory', required=True, type=str, help='Root path for the repo')
@click.option('--env_var', default="Unreal-Engine-Source", help='Environment Variable')
def map_from_environment_variable(root_directory, env_var):
    """Checks for the SOURCE-UNREAL uses the  SOURCE-UNREAL"""
    root_directory = pathlib.Path(root_directory).resolve()
    ue_target_in_repo = pathlib.Path(root_directory).joinpath("UnrealEngine")
 
    engine_source =  os.getenv(env_var)
    
    if engine_source == None:
        print(f"Environment Variable: \"{env_var}\" not found")
        sys.exit(1)

    make_symlink_from_custom_path(ue_target_in_repo,engine_source)

@cli.command()
@click.option('--root_directory', required=True, type=str, help='Root path for the repo')
@click.option('--unreal_version', default="5.0", help='Unreal Engine version')

def map_from_launcher(root_directory, unreal_version):
    """Map Pre-Installed Engine to repo"""
    root_directory = pathlib.Path(root_directory).resolve()
    
    ue_from_registry = pathlib.Path(get_unreal_path_from_registry(unreal_version))
    ue_target_in_repo = pathlib.Path(root_directory).joinpath("UnrealEngine")

    make_symlink_from_custom_path(ue_target_in_repo, ue_from_registry)

if __name__ == '__main__':
    cli()
