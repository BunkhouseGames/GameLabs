load("//:GameLabs/BuildRules/ue5_rules.bzl", "build_client","build_server")

package(default_visibility = ["//visibility:public"])

build_project(
    name="build_me",
    build_script = "//GameLabs/tools:compile_engine.py",
    unreal_engine ="@UnrealEngine//:Setup.bat" 
    )
