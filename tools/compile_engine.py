import sys
import pathlib
import zipfile
import subprocess

input_folder_path = sys.argv[1]
zip_file_name = sys.argv[2]

ue_root_path = pathlib.Path(input_folder_path).parent

directories_to_exclude = [".git", ".vs", "Samples", "FeaturePacks", "Intermediate", "Source"]
extentions_to_exclude = [".pdb"]
_progress_increment = 1000


def should_include_in_archive(filepath):
    """
    Check if the file path conforms with the rules defined in the tool
    """
    for each_parent in filepath.parents:
        if each_parent.name in directories_to_exclude:
            return False

    if filepath.suffix in extentions_to_exclude:
        return False

    return True

def setup_and_compile_engine(input_folder_path):
    subprocess.run([input_folder_path.joinpath("Setup.bat"), "--force"])
    subprocess.run([input_folder_path.joinpath("Engine", "Build", "BatchFiles", "GenerateProjectFiles.bat")])
    subprocess.run([input_folder_path.joinpath("Engine", "Build", "BatchFiles", "RunUBT.bat"), "Win64", "Development", "UE5"])
    pass

def compress_engine(input_folder_path):
    print(input_folder_path)
    import os
    print(os.listdir(input_folder_path))

    files_to_zip = []
    count = 0

    # Loop through the files 
    for i in pathlib.Path(input_folder_path).rglob("*"):
        count = count + 1
        if should_include_in_archive(filepath=i):
            files_to_zip.append(i)

    print(f"Number of files in the original directory: {count} and {len(files_to_zip)} will be added to the compressed file")

    zf = zipfile.ZipFile(zip_file_name, mode='w')


    print(f"Starting to process: {len(files_to_zip)} files")

    for i, e in enumerate(files_to_zip):
        zf.write(e)
        if i % _progress_increment == 0:
            if i != 0:
                percentage = "{:.2f} %".format((i / (len(files_to_zip))) * 100)
                print(f"{percentage}")

    zf.close()

    print("100.00 %")
    print("Finished!")

setup_and_compile_engine(ue_root_path)
compress_engine(ue_root_path)