def _build_impl(ctx):
    out_file = ctx.actions.declare_file("out.zip")

    # engine_root = sys.argv[1]
    # project_file_path = sys.argv[2]
    # profile = sys.argv[3]
    # output_directory = sys.argv[4]
    
    print(ctx.executable.build_tool_path)
    print(ctx.attr.absolute_engine_path)
    print(ctx.attr.absolute_project_path)
    
    ctx.actions.run(
        outputs = [out_file],
        executable = ctx.executable.build_tool_path,
        use_default_shell_env = True,
        arguments = [
            ctx.attr.absolute_engine_path,
            ctx.attr.absolute_project_path,
            "kamo_linux_server",
            "out/"
            ]
    )

    return DefaultInfo(files = depset([out_file]))

build_project = rule (
    implementation = _build_impl,
    attrs = {
        "absolute_engine_path":  attr.string(),
        "absolute_project_path": attr.string(),
        "build_tool_path" : attr.label(executable = True, cfg = "exec"),
    }
)
