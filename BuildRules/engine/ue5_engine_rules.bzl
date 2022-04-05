
def _build_engine_impl(ctx):
    out = ctx.actions.declare_file("out.txt")
    """
    ctx.actions.run(
        outputs = [out],
        executable = attr.file._setup_file,
        arguments = [""]
    )
    """

    return DefaultInfo(files = depset([out]), executable = out)


build_engine = rule (
    implementation = _build_engine_impl,
    attrs = {
        "_setup_file" : attr.label(allow_single_file = True, executable = True, cfg = "exec", default="Setup.bat")
    },
)
