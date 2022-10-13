@echo off
pushd %~dp0\

call build_server.bat
call bump-upload-run-on-aws.bat
call build_game.bat
call upload_to_steam.bat

popd
