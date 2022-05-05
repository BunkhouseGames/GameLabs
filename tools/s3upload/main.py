import subprocess
import boto3
import os

import sys
import pathlib

name = sys.argv[1]
build_target = sys.argv[2]
artifact_folder = sys.argv[3]

BUCKET_NAME = "at-server-builds"
artifact_folder_path = pathlib.Path(artifact_folder).as_posix()

def upload_build():
    name = os.path.split(artifact_folder_path)[-1]

    s3 = boto3.client("s3")

    print(
        f"Uploading server build {artifact_folder_path} to s3://{BUCKET_NAME}/{aws_target}"
    )

    for filename in os.listdir(artifact_folder_path):
        print(os.path.isdir(filename))

        path = artifact_folder_path + "/" + filename
        print(f"Uploading {path} to {aws_target}{filename}")
        s3.upload_file(str(path), BUCKET_NAME, aws_target + filename)


upload_build()
