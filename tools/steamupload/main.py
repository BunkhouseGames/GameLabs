import subprocess
import sys
import pathlib

user_name = sys.argv[1]
password = sys.argv[2]
steam_app_id = sys.argv[3]
steam_depot_id = sys.argv[4]
steam_cli_root = sys.argv[5]
build_to_upload = sys.argv[6]

steam_root = pathlib.Path(steam_cli_root)

BUILD_TO_UPLOAD = pathlib.Path(build_to_upload)
SCRIPT_ROOT_DIRECTORY = pathlib.Path(__file__).parent

def construct_depot_build_file(output_file_path, steam_app_id, steam_depot_id, content_root):

    with open (SCRIPT_ROOT_DIRECTORY.joinpath("depot_build_template"), "r") as template_file_obj:
        contents = template_file_obj.read()
        contents = contents.format(APP_ID=steam_app_id, DEPOT_ID=steam_depot_id, CONTENT_ROOT=content_root)
    
    with open(pathlib.Path(output_file_path), "w") as out_file_obj:
        out_file_obj.write(contents)

def construct_app_build_file(output_file_path, steam_app_id, build_description, steam_depot_id, depot_file_path):

    with open (SCRIPT_ROOT_DIRECTORY.joinpath("app_build_template"), "r") as f:
        contents = f.read()        
        contents = contents.format(APP_ID=steam_app_id, BUILD_DESCRIPTION=build_description, DEPOT_ID=steam_depot_id, DEPOT_FILE_PATH=depot_file_path)

    with open(pathlib.Path(output_file_path), "w") as out_file_obj:
        out_file_obj.write(contents)

def upload_to_steam():

    steam_depot_file_path = steam_root.joinpath(f"depot_build_{steam_depot_id}.vdf")
    steam_app_file_path = steam_root.joinpath(f"app_build_{steam_app_id}.vdf")

    construct_depot_build_file(steam_depot_file_path, steam_app_id, steam_depot_id, BUILD_TO_UPLOAD)
    construct_app_build_file(steam_app_file_path, steam_app_id,"Description of doooom",steam_depot_id, steam_depot_file_path)
 
    subprocess.run([steam_root.joinpath("steamcmd.exe"), f"+login {user_name}", password, f"+run_app_build_http {steam_app_file_path}"])

upload_to_steam()
