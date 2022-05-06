load("@rules_python//python:defs.bzl", "py_binary", "py_library")

py_library(
    name = "GenerateArtifactManifest",
    srcs = [
        "//GameLabs/tools:GenerateArtifactManifest/main.py",
        "//GameLabs/tools:GenerateArtifactManifest/__init__.py"
    ],
    )
py_binary(
  name = "build_game_client",
  main = "//GameLabs/tools:build_game.py",
  srcs = ["//GameLabs/tools:build_game.py"],
)

py_binary(
  name = "build_game_server",
  main = "//GameLabs/tools:build_game.py",
  srcs = ["//GameLabs/tools:build_game.py"],
)

py_binary(
  name = "make-build",
  main = "//GameLabs/tools:make-build.py",
  srcs = ["//GameLabs/tools:make-build.py"],
  deps = ["//GameLabs/tools:GenerateArtifactManifest"]
)

py_binary(
  name = "generate_manifest_file",
  main = "//GameLabs/tools:GenerateArtifactManifest/main.py",
  srcs = ["//GameLabs/tools:GenerateArtifactManifest/main.py"],
)

exports_files(["build_game.py"])