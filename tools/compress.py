import sys
import zipfile
import pathlib

input_folder_path = sys.argv[1]
output_folder_file_path = sys.argv[2]

directories_to_exclude = [".git", ".vs"]
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


files_to_zip = []
count = 0

# Loop through the files 
for i in pathlib.Path(input_folder_path).rglob("*"):
    count = count + 1
    if should_include_in_archive(filepath=i):
        files_to_zip.append(i)

print(f"Number of files in the original directory: {count} and {len(files_to_zip)} will be added to the compressed file")

zf = zipfile.ZipFile(output_folder_file_path, mode='w',compression=zipfile.ZIP_DEFLATED, compresslevel=0)


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
