@echo off
pushd %~dp0\..
python tools\generatemanifest.py %*
popd
