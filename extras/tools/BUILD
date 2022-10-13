load("@rules_python//python:defs.bzl", "py_binary")
load("@pip_install_package//:requirements.bzl", "requirement")

package(default_visibility = ["//visibility:public"])

py_binary(
  name = "build_game",
  srcs = ["build_game.py"],
)

py_binary(
  name = "parse_data_validation",
  srcs = ["parse_data_validation.py"],
)
