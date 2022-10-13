@echo off
pushd %~dp0\..
python tools\uploadbuild.py %*
popd
