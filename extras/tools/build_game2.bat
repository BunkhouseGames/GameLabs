@echo off
pushd %~dp0\

python build_game2.py run-build-action --profile existence_server
call bump-upload-run-on-aws.bat
python build_game2.py run-build-action --profile existence_client

call upload_to_steam.bat

popd
