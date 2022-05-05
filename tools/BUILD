load("@rules_python//python:defs.bzl", "py_binary")

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
  name = "generate_manifest_file",
  main = "//GameLabs/tools:generatemanifest/main.py",
  srcs = ["//GameLabs/tools:generatemanifest/main.py"],
)

exports_files(["build_game.py", "generatemanifest/main.py"])