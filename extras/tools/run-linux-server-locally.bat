@echo off
pushd %~dp0\..
python tools\runlinuxserverlocally.py %*
popd
