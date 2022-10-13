import pathlib
import shutil
import subprocess
import os

def run(args):
    print(f"Running: {' '.join(args)}")
    return subprocess.check_call(args)

def get_uproject_file():
    """Get path to uproject file based """

    project_dir = pathlib.Path().joinpath(pathlib.Path(__file__).parent, "../..").resolve()

    # TODO fix the hundred of things that can go wrong
    for file_path in project_dir.glob('**/*.uproject'):
        return file_path

def run_ue4_test(test_filter="", debug_exe=False, engine_path_overwrite=""):
    
    test_tenant = "automation"
    automation_tenant_path = pathlib.Path(os.path.expanduser("~")).joinpath(".kamo", test_tenant)

    if automation_tenant_path.exists():
        print("Removing the automation tenant from the local database")
        shutil.rmtree(automation_tenant_path)

    if debug_exe:
        executable = "UE4Editor-Win64-Debug-Cmd.exe"
    else:
        executable = "UE4Editor-Cmd.exe"
    
    # Project Path
    project_dir = get_uproject_file()
    
    # Overwriting the path if needed
    if engine_path_overwrite:
        engine_root = pathlib.Path(engine_path_overwrite)
        engine_exe = pathlib.Path(str(engine_root) + f'/Engine/Binaries/Win64/{executable}')
    else:
        engine_root = pathlib.Path().joinpath(pathlib.Path(__file__).parent, f'UnrealEngine/')
        engine_exe = pathlib.Path().joinpath(pathlib.Path(__file__).parent, f'UnrealEngine/Engine/Binaries/Win64/{executable}').resolve()

    if not engine_exe.exists():
        print(f"Unreal Engine not found at : {str(engine_exe)}")
        return
    elif not project_dir.exists():
        print(f"Project not found at : {str(project_dir)}")
        return

    logs_to_turn_off = ["LogConfig", "LogModuleManager", "LogTargetPlatformManager", "LogSlate", "LogDemo", "LogHoudiniEngine", "LogAudioMixer", "LogAudio", "SourceControl", "LogDerivedDataCache","LogConfig", "LogMemory", "LogInit", "LogMemory","LogWindows",
    "LogShaderCompilers","LogTextLocalizationManager","LogMeshReduction","LogMeshMerging","LogAISub"]
    
    log_filter = ""
    for e in logs_to_turn_off:
        log_filter = log_filter + e + " off,"

    empty_map = "/Game/Tests/Map_Empty"
    # extra_arguments = ["-buildmachine", "-nullRHI", "-UNATTENDED","-game", "-log", "-WARNINGSASERRORS"]

    #"--ExecCmds=Automation RunTests Existence.Login", "-testexit=Automation Test Queue Empty",
    cmds = [str(engine_exe), f"{project_dir}", empty_map,f"-kamotenant={test_tenant}","-buildmachine" ,"-nullRHI", "-UNATTENDED","-game", "-log", "-LogCmds=" + log_filter,
     f"--ExecCmds=Automation RunTests {test_filter}", "-testexit=Automation Test Queue Empty"]

    run(cmds)

run_ue4_test(test_filter="Kamo.Create", engine_path_overwrite=f"C:/Work/Epic/UE_4.26")
