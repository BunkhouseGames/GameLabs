@echo off
pushd %~dp0\..

.\UnrealEngine\Engine\Binaries\Win64\UnrealEditor-Win64-DebugGame-Cmd.exe D:\work\ProjectDawn\ProjectDawnGame\ProjectDawnPreview.uproject -run=Cook -Map=Map_Landscape_Playground+Map_Landscape_Playground -CookCultures=en -TargetPlatform=WindowsServer -fileopenlog -ddc=DerivedDataBackendGraph -unversioned -compressed -stdout -CrashForUAT -unattended -NoLogTimes  -UTF8Output


popd
