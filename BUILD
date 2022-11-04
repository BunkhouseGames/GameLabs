load("@rules_python//python:defs.bzl", "py_binary")
load("@pip_deps//:requirements.bzl", "requirement")

py_binary(
  name = "add_project",
  srcs = ["add_project.py"],
  deps = [requirement("cookiecutter")]
)