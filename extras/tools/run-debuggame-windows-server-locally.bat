@echo off
pushd %~dp0\..

"C:\Program Files\Epic Games\UE_5.0\Engine\Binaries\Win64\UnrealEditor-Win64-DebugGame.exe" "D:\work\ProjectDawn\ProjectDawnGame\ProjectDawnPreview.uproject" /Game/ProjectDawn/Levels/Map_ProjectDawn_P.Map_ProjectDawn_P -server -log -port=7777

popd
