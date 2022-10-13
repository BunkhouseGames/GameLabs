@echo off
pushd %~dp0\..
kamo jobs create -y --map Map_Login_P %*
popd
