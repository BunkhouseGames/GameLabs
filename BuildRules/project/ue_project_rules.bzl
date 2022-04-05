def _build_impl(ctx):
    out_file = ctx.actions.declare_file("out.zip")

    ctx.actions.run(
        outputs = [out_file],
        executable = "python",
        arguments = [
            ctx.file.build_script.path,
            ctx.file.unreal_engine.path,
            out_file.path]
    )

    return DefaultInfo(files = depset([out_file]))

build_project = rule (
    implementation = _build_impl,
    attrs = {
        "unreal_engine":  attr.label(allow_single_file = True),
        "build_script": attr.label(allow_single_file = True)
    }
)
