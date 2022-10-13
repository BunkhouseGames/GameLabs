import json
import os
import pathlib
import subprocess
import shutil
import random
import time
import pytest
import redis

from pathlib import Path


def get_uproject_file():
    """Get path to uproject file based """

    # HACK: Assuming project is three stories above us.
    project_dir = pathlib.Path().joinpath(pathlib.Path(__file__).parent, "../../..").resolve()

    print(f"Searching for project in: {project_dir}")
    # TODO fix the hundred of things that can go wrong
    for file_path in project_dir.glob('**/*.uproject'):
        return file_path

    raise RuntimeError("Error: No project found!")


def run_ue4_test(test_filter: str = "Existence",
                 debug_exe: bool = False,
                 kamo_driver: str = "file",
                 kamo_redis_url: str = None,
                 log_output=None):
    print("Running UE4 tests")
    test_tenant = "automation"
    if debug_exe:
        executable = "UE4Editor-Win64-Debug-Cmd.exe"
    else:
        executable = "UE4Editor-Cmd.exe"

    # Project Path
    project_dir = get_uproject_file()

    unreal_path = os.environ.get("UNREALENGINEFOLDER", "")
    if unreal_path:
        engine_root = Path(unreal_path)
        engine_exe = Path(str(engine_root) + f'/Engine/Binaries/Win64/{executable}')
    else:
        engine_exe = Path().joinpath(Path(__file__).parent,
                                     f'UnrealEngine/Engine/Binaries/Win64/{executable}').resolve()

    assert engine_exe.exists(), f"Unreal Engine not found at : {str(engine_exe)}"
    assert project_dir.exists(), f"Project not found at : {str(project_dir)}"

    logs_to_turn_off = ["LogConfig", "LogModuleManager", "LogTargetPlatformManager", "LogSlate", "LogDemo",
                        "LogHoudiniEngine", "LogAudioMixer", "LogAudio", "SourceControl", "LogDerivedDataCache",
                        "LogConfig", "LogMemory", "LogInit", "LogMemory", "LogWindows",
                        "LogShaderCompilers", "LogTextLocalizationManager", "LogMeshReduction", "LogMeshMerging",
                        "LogAISub", "LogLinker"]

    log_filter = " off, ".join(logs_to_turn_off)

    cmds = [
        str(engine_exe),
        f"{project_dir}",
        "/Kamo/Maps/KamoTestMap",
        f"--ExecCmds=Automation RunTests {test_filter};quit",
        f"-kamotenant={test_tenant}",
        "-game",
        "-server",
        "-stdout",
        "-nosplash",
        "-unattended",
        "-nullRHI",
        f"-LogCmds={log_filter}",
        "-testexit=Automation Test Queue Empty",
        f"-kamodriver={kamo_driver}",
        "-kamoclassmap=/Kamo/KamoClassMap.KamoClassMap"
    ]

    if kamo_redis_url is not None:
        cmds.append(f"-kamoredisurl={kamo_redis_url}")

    run(cmds, log_output=log_output)


def run(args, log_output=None):
    print(f"Running: {' '.join(args)}")
    return subprocess.check_call(args, stdout=log_output, stderr=log_output)


@pytest.fixture
def run_redis_server(pytestconfig):
    # Run Redis server in Docker on a random port
    port = random.randrange(10000, 25000)
    container_name = f"kamotest-redisserver-{port}"
    # this takes too long: ret = run(f"docker pull redis:latest")
    run(f"docker run -d -p {port}:6379 --name {container_name} redis:latest")
    pytestconfig.cache.set("redis/port", port)
    redis_url = f"redis://127.0.0.1:{port}/0"
    pytestconfig.cache.set("redis/url", redis_url)

    time.sleep(1)
    yield redis_url

    run(f"docker stop {container_name}")
    run(f"docker rm {container_name}")


def test_redis_server(run_redis_server):
    """
    Test that the redis server runs and is reachable
    """
    r = get_redis_from_url(run_redis_server)
    assert r.dbsize() == 0, "Database is not empty"


def test_existence():
    """
    Ensures that the Unreal testing framework is working
    """
    with open("unreal_tests.unreal.log", "w") as f:
        run_ue4_test(test_filter="Existence", log_output=f)


def test_kamo_unreal_tests():
    """
    Basic Kamo tests that run inside Unreal
    """
    with open("unreal_tests.unreal.log", "w") as f:
        run_ue4_test(test_filter="Kamo.KamoState", log_output=f)


def test_kamo_filedb_create_objects():
    """
    Create test objects and check the file db directly
    """
    clear_kamo_file_db()
    with open("filedb_create_objects.unreal.log", "w") as f:
        run_ue4_test(test_filter="KamoDbTest.CreateTestObjects", log_output=f)
    validate_kamo_file_db()


def test_kamo_filedb_objects_recreated():
    """
    Create test objects and validate objects in Unreal in a second run, using a file db
    """
    clear_kamo_file_db()
    with open("filedb_objects_recreated.unreal.log", "w") as f:
        run_ue4_test(test_filter="KamoDbTest.CreateTestObjects", log_output=f)
        run_ue4_test(test_filter="KamoDbTest.ValidateTestObjects", log_output=f)


def test_kamo_filedb_objects_deleted():
    """
    Create test objects and delete them again in second run, using a file db
    """
    clear_kamo_file_db()
    with open("filedb_objects_deleted.unreal.log", "w") as f:
        run_ue4_test(test_filter="KamoDbTest.CreateTestObjects", log_output=f)
        run_ue4_test(test_filter="KamoDbTest.DeleteTestObjects", log_output=f)
    validate_kamo_file_db_empty()


def test_kamo_redisdb_create_objects(run_redis_server):
    """
    Create test objects and check the redis db directly
    """
    r = get_redis_from_url(run_redis_server)
    clear_kamo_redis_db(r)
    with open("redisdb_create_objects.unreal.log", "w") as f:
        run_ue4_test(test_filter="KamoDbTest.CreateTestObjects", kamo_driver="redis", kamo_redis_url=run_redis_server,
                     log_output=f)
    validate_kamo_redis_db(r)


def test_kamo_redisdb_objects_recreated(run_redis_server):
    """
    Create test objects and validate objects in Unreal in a second run, using a redis db
    """
    with open("redisdb_objects_recreated.unreal.log", "w") as f:
        run_ue4_test(test_filter="KamoDbTest.CreateTestObjects", kamo_driver="redis", kamo_redis_url=run_redis_server,
                     log_output=f)
        run_ue4_test(test_filter="KamoDbTest.ValidateTestObjects", kamo_driver="redis", kamo_redis_url=run_redis_server,
                     log_output=f)


def test_kamo_redisdb_objects_deleted(run_redis_server):
    """
    Create test objects and delete them again in second run, using a redis db
    """
    r = get_redis_from_url(run_redis_server)
    clear_kamo_redis_db(r)
    with open("redisdb_objects_deleted.unreal.log", "w") as f:
        run_ue4_test(test_filter="KamoDbTest.CreateTestObjects", kamo_driver="redis", kamo_redis_url=run_redis_server,
                     log_output=f)
        time.sleep(30)
        run_ue4_test(test_filter="KamoDbTest.DeleteTestObjects", kamo_driver="redis", kamo_redis_url=run_redis_server,
                     log_output=f)
    validate_kamo_redis_db_empty(r)


def clear_kamo_file_db():
    folder = get_kamo_file_db_path()
    if folder.exists():
        print(f"Deleting {folder}")
        shutil.rmtree(folder)
    else:
        print(f"{folder} does not exist")


def get_kamo_file_db_path():
    return Path.home().joinpath(".kamo", "automation", "db", "region.kamotestmap")


def validate_default_object(obj):
    assert (obj["IntProp"] == 42)
    assert (obj["StringProp"] == "This is a test")
    assert (obj["StringMapProp"] == {
        "keys": [],
        "values": []
    })
    assert (len(obj["StringSetProp"]) == 0)


def validate_changed_object(obj):
    assert (obj["IntProp"] == -37)
    assert (obj["StringProp"] == "This has been changed")
    assert (obj["StringMapProp"] == {
        "keys": [
            "bingo",
            "foo"
        ],
        "values": [
            "bongo",
            "bar"
        ]
    })
    assert (len(obj["StringSetProp"]) == 4)


def validate_object(obj):
    if obj["StringProp"] == "This is a test":
        validate_default_object(obj)
    else:
        validate_changed_object(obj)


def validate_kamo_file_db():
    folder = get_kamo_file_db_path()
    assert (folder.exists())
    object_files = list(folder.glob("test.*.json"))
    assert (len(object_files) == 2)
    for each in object_files:
        with open(each) as f:
            obj = json.load(f)
            validate_object(obj)


def validate_kamo_file_db_empty():
    folder = get_kamo_file_db_path()
    assert folder.exists(), "Database does not exist"
    object_files = list(folder.glob("test.*.json"))
    assert len(object_files) == 0, "Database is not empty"


def clear_kamo_redis_db(r):
    for k in r.scan_iter("ko:automation:*"):
        r.delete(k)


def validate_kamo_redis_db(r):
    for k in r.scan_iter("ko:automation:db:region.kamotestmap:*"):
        s = r.get(k)
        obj = json.loads(s)
        validate_object(obj)


def validate_kamo_redis_db_empty(r):
    for k in r.scan_iter("ko:automation:db:region.kamotestmap:*"):
        assert False, f"Database is not empty - found key {k}"


def get_redis_from_url(redis_url: str) -> redis.Redis:
    host_port_db = redis_url.replace("redis://", "")
    host, portdb = host_port_db.split(":")
    port, db = portdb.split("/")
    print(host, port, db)
    r = redis.Redis(host=host, port=port, db=db)
    return r
