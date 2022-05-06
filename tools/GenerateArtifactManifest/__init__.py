import subprocess
import datetime


def get_sha():
    ret = subprocess.check_output(
        ["git", "rev-parse", "--short", "HEAD"], universal_newlines=True
    ).strip()
    return ret

def get_branch():
    return subprocess.check_output(["git", "rev-parse", "--abbrev-ref", "HEAD"], universal_newlines=True).strip()

def get_version():
    return "Dawn_" + str(datetime.datetime.utcnow().isoformat()) +"_"+ get_sha() 

def get_user_name():
    return "Not-Set"
