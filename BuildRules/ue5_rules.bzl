def _build_client_impl(ctx):

    # Declare the file that will be executed
    executable = ctx.actions.declare_file("run.bat")
    out = ctx.actions.declare_file("output.out")
    # print(ctx.files)

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

    ctx.actions.write(
        output=executable,
        content="python --version",
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

    # Write data into the file
    # f = ctx.attr.deps[0].files.to_list()
    # print(f)
    # print(type(ctx.attr.deps[0]))
    # ["data_runfiles", "default_runfiles", "files", "files_to_run", "label"]

    ctx.actions.write(
        output=executable,
        content="python --version",
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