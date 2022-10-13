"""
Replay git commits from one git repository into another unrelated git repository.

This uses the git format-patch and am command which "Prepare patches for e-mail submission" and
"Apply a series of patches from a mailbox".

The relevant commit hashes are fetched from the source repo, a patch created and applied in the
destination repo.

Example:

> git -C ../ProjectDawn log --pretty=format:"%H"  ProjectDawnGame/Plugins/ExistenceSDK/Kamo
2a03820ba9fe4fe0c03cb931e30112ca3935c846
5469b9aade756bbb51dd34416e78abe2bddc9831
...

> git --git-dir=../ProjectDawn/.git format-patch -k -1 --stdout 2a03820ba9fe4fe0c03cb931e30112ca3935c846 | git am -3 -k


(c) Arctic Theory - Matti
"""
import os
import sys
import subprocess
import string
import random


def replay_git_commits(path, dest_repo_root, from_hash):
    # Abort any current am session
    cmd = f"git -C {dest_repo_root} am --abort"
    print(f"Running: {cmd}")
    subprocess.run(cmd)

    # Get all the hashes
    cmd = f"git -C ../ log --pretty=format:\"%H %as %s\" {path}"
    print(f"Running: {cmd}")
    output = subprocess.check_output(cmd, universal_newlines=True)
    if from_hash not in output:
        output = output[:3000]
        print(f"Starting hash not found in history. Aborting\n\nHistory:\n\n{output}...")
        sys.exit(1)

    # A line has hash, date and subject, example:
    # 049a0ae18e6da5f0025ad622e530e8ec66e26e13 2022-09-07 Streamline Kamo Runtime startup logic
    hashes = [line.split(" ")[0] for line in output.splitlines()]
    hashes.reverse()

    del hashes[:hashes.index(from_hash)]  # Strip up until the starting hash

    print(f"Replaying {len(hashes)} commits to {dest_repo_root}")

    # Apply all the commits
    for commit_hash in hashes:
        # Create the patch
        cmd = f"git format-patch -k -1 --relative={path} --stdout {commit_hash}"
        print(f"Running: {cmd}")
        patch = subprocess.check_output(cmd, universal_newlines=True)

        ## LFS filenames contain absolute path from the source repository. We must strip the 'path' from the front:
        ##processed_patch = patch.replace("/" + path, "")

        # Parse out LFS objeft references and migrate them over to destination repository
        for lfs_object_id in get_lfs_objectids_from_patch(patch):
            migrate_lfs_object(lfs_object_id, dest_repo_root)

        # Apply the patch using 'am' or Apply Mailbox command
        header = patch.split("---\n", 1)[0]
        print(f"Applying patch:\n{header}")

        cmd = f"git -C {dest_repo_root} am -3 -k --ignore-whitespace"
        print(f"Running: {cmd}")
        process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stdin=subprocess.PIPE, stderr=subprocess.PIPE)
        patch_result, error = process.communicate(input=patch.encode("ascii"))

        # See if the patch got applied correctly
        patch_result = patch_result.decode("ascii").strip()
        error = error.decode("ascii").strip()
        if patch_result:
            print(f"Patch result: {patch_result}")
        if error:
            print(f"Error output: {error}")
            if error.lower().startswith("error") or error.lower().startswith("fatal"):
                print("Replay stopped due to an error.")
                sys.exit(1)

        print("\n\n")


def get_lfs_objectids_from_patch(patch):
    """
    Returns a list of all LFS object id's referenced in 'patch'.


    Anatomy of an LFS patch:

    diff --git a/ProjectDawnGame/Plugins/ExistenceSDK/Kamo/Content/UI/KamoClient/UI_KamoClientInfo.uasset b/ProjectDawnGame/Plugins/ExistenceSDK/Kamo/Content/UI/KamoClient/UI_KamoClientInfo.uasset
    new file mode 100644
    index 000000000..c0b9961b8
    --- /dev/null
    +++ b/ProjectDawnGame/Plugins/ExistenceSDK/Kamo/Content/UI/KamoClient/UI_KamoClientInfo.uasset
    @@ -0,0 +1,3 @@
    +version https://git-lfs.github.com/spec/v1
    +oid sha256:f6025ac21b8135a66cd8fe1c60d19a53cce77ab5df3051a3977b6ebae130b0f0
    +size 59663

    The trick is to parse out any "+oid sha256:<object_id>"
    """
    object_ids = []
    for line in patch.splitlines():
        if "oid sha256:" in line:
            prefix, object_id = line.split("oid sha256:", 2)
            object_ids.append(object_id)

    return object_ids


def migrate_lfs_object(lfs_object_id, dest_repo_root):
    """Migrate an LFS object to an unrelated repo."""

    remote_tmp_name = "lfs-migration-" + ''.join(random.choices(string.ascii_lowercase + string.digits, k=5))
    cmd = f"git remote add {remote_tmp_name} {dest_repo_root}"
    print(f"Running: {cmd}")
    subprocess.run(cmd)
    try:
        cmd = f"git lfs push --object-id {remote_tmp_name} {lfs_object_id}"
        print(f"Running: {cmd}")
        subprocess.run(cmd).check_returncode()  # Bail out if this fails
    finally:
        cmd = f"git remote remove {remote_tmp_name}"
        print(f"Running: {cmd}")
        subprocess.run(cmd)


if __name__ == "__main__":
    if len(sys.argv) != 4:
        print("Usage: replay-git-commits.py <git path to files> <destination repo root> <from hash>")
        sys.exit(1)
    path = sys.argv[1]
    dest_repo_root = sys.argv[2]
    from_hash = sys.argv[3]
    os.path.join(dest_repo_root, ".git")
    replay_git_commits(path, dest_repo_root, from_hash)
