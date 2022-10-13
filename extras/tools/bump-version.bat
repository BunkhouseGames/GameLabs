@echo off
pushd %~dp0\..
call python tools/bumpversion.py %*
call python tools/releasenotes.py %*
popd
