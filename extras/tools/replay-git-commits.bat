@echo off
pushd %~dp0
call python replay-git-commits.py "ProjectDawnGame/Plugins/ExistenceSDK/Kamo" ..\..\kamo 785f042e845834e0648f0c7e6786ef92b1a181a4
popd
