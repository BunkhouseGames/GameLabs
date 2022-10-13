#!/usr/bin/env python3
"""
Upload Linux build to S3 archive.
"""
import os.path
import yaml
import json
import subprocess
from glob import glob
import re
import sys
import io
import shutil
import zipfile
from tempfile import NamedTemporaryFile
import urllib.parse
import time
from collections import OrderedDict


try:
    import boto3
except ImportError as e:
    boto3 = None


# Todo: Add this to kamo config
CDN_ROOT = "https://builds.arctictheory.com/"
CLOUDFRONT_DISTRIBUTION_ID = "E2WOQPBJTXVQ62"




def get_config():
    config_file = os.path.join(os.path.expanduser("~"), ".kamo", "config.yaml")
    with open(config_file) as f:
        return yaml.safe_load(f)


def get_projects(search_from="."):
    """Search for .uproject files and return absolute path to them."""

    globresult = glob(search_from + "/*/*.uproject")
    if not globresult:
        root_dir = os.path.abspath(search_from)
        raise RuntimeError(f"Error: No *.uproject file found in any subfolder of {root_dir}")

    project_files = [os.path.abspath(path) for path in globresult]
    return project_files


def _sanitize_s3_builds_path(s3_builds_path):
    if s3_builds_path is None:
        config = get_config()
        default_config = config["profiles"][config["default_profile"]]
        s3_builds_path = default_config["s3_builds_path"]

    # Strip trailing slash if needed
    if s3_builds_path.endswith("/"):
        s3_builds_path = s3_builds_path[:-1]

    return s3_builds_path


def upload_build(s3_builds_path=None):
    """Upload all builds found in <Project>/Saved/StagedBuilds to S3."""

    s3_builds_path = _sanitize_s3_builds_path(s3_builds_path)

    # Find UE project file which gives us the project root folder.
    project_files = get_projects(".")
    project_root, _ = os.path.split(project_files[0])

    # Find all builds in Saved/StagedBuilds
    project_files = get_projects(os.path.join(project_root, "Saved", "StagedBuilds", "*"))
    print(f"UE build artifact uploader. Uploading {len(project_files)} target(s):\n\n")

    # Keep track of which indexes need to be rebuilt for client builds.
    rebuild_indexes = set()

    for project_file in project_files:
        project_root, project_filename = os.path.split(project_file)

        # The filename of the .uproject file is the canonical name of the project (UE Idiosyncracy)
        project_name, _= os.path.splitext(project_filename)
        build_root = os.path.abspath(os.path.join(project_root, ".."))

        # Parent folder name is the same as the target name
        _, target_name = os.path.split(build_root)

        # Version is stored in DefaultGame.ini
        game_ini_file = os.path.join(project_root, "Config", "DefaultGame.ini")
        with open(game_ini_file) as f:
            result = re.search("ProjectVersion=(?P<version>.*)", f.read())
            if not result:
                print(f"Error: Can't find 'ProjectVersion' in .ini file: {game_ini_file}")
                continue
            version = result.groupdict()["version"]

        # Server builds are uploaded to S3 unarchived. Client builds are archived into a zip file.
        # There is no sure way to distinguish server builds from client builds but we do generate
        # a 'buildinfo.json' file for the server builds so that will be our logic:
        is_server = os.path.exists(os.path.join(build_root, "buildinfo.json"))

        # Upload path or archive name:
        # For non-client builds: <s3 root>/<project name>/<target>/<version>/[build artifact files...]
        # For client builds:     <s3 root>/<project name>/<target>/<version>/<project name>.<target>-<version>.zip
        if is_server:
            object_path = f"/{project_name}/{target_name}/{version}/"
            s3_target_path = f"{s3_builds_path}{object_path}"
        else:
            object_path = f"/{project_name}/{target_name}/{version}/{project_name}.{target_name}-{version}.zip"
            s3_target_path = f"{s3_builds_path}{object_path}"

        print(f"---------------------------------------------------------")
        print(f"Project:          {project_name}")
        print(f"Build root:       {build_root}")
        print(f"Target name:      {target_name}")
        print(f"Project version:  {version}")
        print(f"Object path   :   {object_path}")
        print(f"S3 target path:   {s3_target_path}")
        print(f"Target type:      {'server' if is_server else 'client'}")
        print(f"")

        if is_server:
            cmd = ["aws", "s3", "sync", build_root, s3_target_path]
            print("Executing upload command: " + " ".join(cmd))
            #subprocess.run(cmd, shell=True)

            # Grab the output so we can prune it a bit (only show bytes completed, not which files got uploaded)
            process = subprocess.Popen(cmd, stderr=subprocess.STDOUT, stdout=subprocess.PIPE)
            buff = io.BytesIO()

            for c in iter(lambda: process.stdout.read(1), b""):
                buff.write(c)
                if c == b'\r':
                    if b"upload" not in buff.getvalue():
                        line = buff.getvalue().decode(sys.stdout.encoding).replace("\n", "")
                        print(line, end="", flush=True)
                    buff = io.BytesIO()
        else:
            # For client builds we archive it into a compressed zip file and move it to S3
            print(f"Archiving client build...")
            with NamedTemporaryFile(delete=False) as tempfile:
                with zipfile.ZipFile(tempfile, "w", compression=zipfile.ZIP_DEFLATED) as zipf:
                    total_size = zip_dir(build_root, zipf)

            print(f"Archive size: {int(total_size / 1000000)} MB.")

            cmd = ["aws", "s3", "rm", s3_target_path]
            print("Executing delete command: " + " ".join(cmd))
            subprocess.run(cmd)

            cmd = ["aws", "s3", "mv", tempfile.name, s3_target_path]
            print("Executing upload command: " + " ".join(cmd))
            subprocess.run(cmd)
            print(f"Done archiving client build.")

            # Re-create index for client zip files
            _create_index(s3_builds_path, project_name, target_name)

            # Invalidate build on Cloudfront
            _cloudfront_invalidate(CLOUDFRONT_DISTRIBUTION_ID, object_path)


    print(f"---------------------------------------------------------\n")



def _create_index(s3_builds_path, project_name, target_name):
    """Create index for project <project_name> for all builds in 's3_builds_path'"""

    if not boto3:
        print(f"Error: Can't import boto3, can't create index\n{e}")
        return

    print(f"|")
    print(f"|Note: The client builds are Cloudfront'ed but access to S3 bucket contents is controlled through Bucket Policy.")
    print(f"|      Make sure to whitelist projects and targets to make them available through Cloudfront endpoint.")
    print(f"|")
    print(f"Creating indexes...")

    s3 = boto3.resource("s3")
    bucket_name = urllib.parse.urlparse(s3_builds_path).netloc

    print(f"Indexing {project_name}.{target_name}")
    s3_prefix = f"{project_name}/{target_name}/"
    files = s3.Bucket(bucket_name).objects.filter(Prefix=s3_prefix).all()
    versions = {}
    for file in files:
        # Important: We only index zip archives.
        if not file.key.lower().endswith(".zip"):
            continue

        # Version is the third folder
        version = file.key.split("/")[2]
        versions[version] = {
            "project_name": project_name,
            "target_name": target_name,
            "version": version,
            "url": CDN_ROOT + urllib.parse.quote(file.key),
            "s3":
            {
                "s3_uri": f"{s3_builds_path}/{file.key}",
                "key": file.key,
                "size": file.size,
                "last_modified": file.last_modified.isoformat(),
                "e_tag": file.e_tag,
                #"owner": file.owner,  # Sensitive info?
            }
        }


    # Note: Sorting semantic versions is not really a straight forward thing
    sorted_versions = _sort_semantic_versions(versions.keys())
    versions = [versions[ver] for ver in sorted_versions]

    index_file_path = f"{s3_prefix}index.json"
    index_file = s3.Object(bucket_name, index_file_path)
    index_doc = {
        "project_name": project_name,
        "target_name": target_name,
        "latest_version": versions[0] if versions else None,
        "versions": versions,

    }
    json_index = json.dumps(index_doc, indent=4)
    index_file.put(Body=json_index, ContentType="application/json")
    print(f"Creating index file {index_file.key}:\n{json_index}")

    # Invalidate Cloudfront cache for index file
    _cloudfront_invalidate(CLOUDFRONT_DISTRIBUTION_ID, "/" + index_file_path)


def _sort_semantic_versions(versions):
    """Sort a list containing semantic versions, from highest to lowest."""

    def get_version_tuple(ver):
        PATTERN = r"^(?P<major>0|[1-9]\d*)\.(?P<minor>0|[1-9]\d*)\.(?P<patch>0|[1-9]\d*)(?:-(?P<prerelease>(?:0|[1-9]\d*|\d*[a-zA-Z-][0-9a-zA-Z-]*)(?:\.(?:0|[1-9]\d*|\d*[a-zA-Z-][0-9a-zA-Z-]*))*))?(?:\+(?P<buildmetadata>[0-9a-zA-Z-]+(?:\.[0-9a-zA-Z-]+)*))?$"
        search_result = re.search(PATTERN, ver)
        if not search_result:
            print(f"Warning: Not a semantic version: {semantic_version}. See https://semver.org/")
        else:
            parts = search_result.groupdict()
            return int(parts["major"]), int(parts["minor"]), int(parts["patch"]), parts["prerelease"] or "", parts["buildmetadata"] or ""

    return sorted(versions, key=get_version_tuple, reverse=True)


def _cloudfront_invalidate(distribution_id:str, path:str='/*')->str:
  '''
    create a cloudfront invalidation
    parameters:
      distribution_id:str: distribution id of the cf distribution
      path:str: path to invalidate, can use wildcard eg. "/*" means all path
    response:
      invalidationId:str: invalidation id
  '''
  print(f"Invalidating cloudfront path {path} on distribution {distribution_id}")
  cf = boto3.client('cloudfront')
  if not boto3:
    print(f"Warning: Can't invalidate, boto3 module not installed.")
    return

  # Create CloudFront invalidation
  res = cf.create_invalidation(
      DistributionId=distribution_id,
      InvalidationBatch={
          'Paths': {
              'Quantity': 1,
              'Items': [
                  urllib.parse.quote(path)  # Yes this is necessary
              ]
          },
          'CallerReference': str(time.time()).replace(".", "")
      }
  )
  invalidation_id = res['Invalidation']['Id']
  return invalidation_id



def zip_dir(path, ziph):
    # ziph is zipfile handle
    totalsize = 0
    linelength = 0

    for root, dirs, files in os.walk(path):
        for file in files:
            filename = os.path.join(root, file)
            totalsize += os.stat(filename).st_size

            # Print out progress
            progress = f"Archived {int(totalsize / 1000000)} MB. Adding {file}"
            padding = " " * (linelength - len(progress))
            linelength = len(progress)
            print(f"{progress}{padding}\r", end="", flush=True)

            # Add file to zip archive
            ziph.write(filename, os.path.relpath(filename, os.path.join(path, '..')))

            ##if totalsize > 5000000: break

    print(" " * linelength)

    return totalsize


if __name__ == "__main__":
    upload_build()

