
def _build_impl(ctx):
    pass

build_project = rule (
    implementation = _build_impl,
    attrs = {
        "unreal_engine": attr.label(allow_files = True),
    }
)
