def _build_engine_impl(ctx):
    out = ctx.actions.declare_file("out.txt")

    ctx.actions.run(outputs=[out], executable=attr.file._setup_file, arguments=[""])

    return DefaultInfo(files=depset([out]), executable=out)


def _install_source_engine(ctx):
    file = ctx.actions.declare_file("my_out.bat")
    print(dir(ctx.executable.python_installation))
    print(ctx.executable.python_installation.dirname)

    cmd = ctx.executable.python_installation.short_path 
    ctx.actions.write(
        output = file,
        content = cmd.replace("/","\\"),
        is_executable = True
    )

    return DefaultInfo(executable = file)


install_source_engine = rule(
    implementation=_install_source_engine,
    executable = True,
    attrs={
        "python_installation" : attr.label(
            executable = True,
            cfg = "exec",
            allow_files = True),
        # "unreal_engine" : attr.label(allow_files = True),
        "output_root" : attr.string(),
    }
)

build_engine = rule(
    implementation=_build_engine_impl,
    attrs={
        "_setup_file": attr.label(
            allow_single_file=True, executable=True, cfg="exec", default="Setup.bat"
        )
    },
)
