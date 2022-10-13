@echo off
pushd %~dp0\..

"D:\work\ProjectDawn\ProjectDawnGame\Binaries\Win64\ProjectDawnPreviewServer-Win64-Debug.exe" /Game/ProjectDawn/Levels/Map_ProjectDawn_P.Map_ProjectDawn_P -server -log -port=7777

popd
