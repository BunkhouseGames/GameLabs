@echo off
pushd %~dp0\..
call python tools/releasenotes.py %*
popd
