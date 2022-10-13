pushd %~dp0
kamo -t staging jobs shutdown-all -y

call bump-version.bat
call generate-manifest.bat

aws configure set default.s3.max_concurrent_requests 64
aws configure set default.s3.multipart_threshold 4MB
aws configure set default.s3.multipart_chunksize 4MB
call upload-build.bat

call launch-current-server-ppv.bat

popd
