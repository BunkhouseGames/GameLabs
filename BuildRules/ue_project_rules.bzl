
def _build_impl(ctx):
    pass

build_project = rule (
    implementation = _build_impl,
    attrs = {
        "unreal_engine": attr.label(executable = True, cfg = "exec",allow_files = True),
    }
)
