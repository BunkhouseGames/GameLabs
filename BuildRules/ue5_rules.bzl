
def _build_client_impl(ctx):

    # Declare the file that will be executed
    executable = ctx.actions.declare_file("run.bat")
    out = ctx.actions.declare_file("output.out")
    # print(ctx.files)

    # C:\Work\Epic\UE_5.0EA

    ctx.actions.run(
        outputs = [out],
        inputs = ctx.files.srcs,
        executable = "python --version"
    )

    # Write data into the file
    # f = ctx.attr.deps[0].files.to_list()
    # print(f)
    # print(type(ctx.attr.deps[0]))
    # ["data_runfiles", "default_runfiles", "files", "files_to_run", "label"]

    ubt = "C:/Work/Epic/UE_5.0EA/Engine/Binaries/DotNET/UnrealBuildTool/UnrealBuildTool.exe"
    ctx.actions.write(
        output=executable,
        content=ubt,
        is_executable = True,
        )

    # Returns in a way that is executable
    return DefaultInfo(executable  = executable)

def _build_server_impl(ctx):

    # Declare the file that will be executed
    executable = ctx.actions.declare_file("run.bat")
    out = ctx.actions.declare_file("output.out")
    # print(ctx.files)

    ctx.actions.run(
        outputs = [out],
        inputs = ctx.files.srcs,
        executable = "python --version"
    )

    # TODO the project file should be passed in through ctx.files.srcs so we should be able to get this from there
    project_path = "C:/Work/GameLabs/GameLabs/_generated_projects/NewTest/NewTestGame/NewTestGame.uproject"

    # TODO the UBT tool (and UE in general) should be dealt with as a dependancy and passed in as like that
    ubt = "C:/Work/Epic/UE_5.0EA/Engine/Binaries/DotNET/UnrealBuildTool/UnrealBuildTool.exe"

    build_arguments = "Development Win64 -TargetType=Editor -progress -NoHotReloadFromIDE"
 
    cmd = ubt + " " + project_path + " " + build_arguments

    ctx.actions.write(
        output=executable,
        content=cmd,
        is_executable = True,
        )

    # Returns in a way that is executable
    return DefaultInfo(executable  = executable)

build_client = rule (
    implementation = _build_client_impl,
    executable = True,
    attrs = {
        "srcs": attr.label_list(allow_files = True)
        }
)

build_server = rule (
    implementation = _build_server_impl,
    executable = True,
    attrs = {
        "srcs": attr.label_list(allow_files = True)
        }
)