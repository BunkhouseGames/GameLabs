@echo off
pushd %~dp0\..
kamo -t staging nodes ensure 1
kamo -t staging jobs shutdown-all -y
kamo -t staging jobs create -y --map Map_Login_P -a "-SessionOwner=%USERNAME%" %*
rem kamo -t staging jobs create -y --map Map_RiverValley --ppv -a "-SessionOwner=%USERNAME%,-beaconport=0" %*
rem kamo -t staging jobs create -y --map Map_Landscape_Playground --ppv -a "-SessionOwner=%USERNAME%,-beaconport=0" %*
kamo -t staging jobs create -y --map Map_BeautifulCorner_01 --ppv -a "-SessionOwner=%USERNAME%,-beaconport=0" %*
popd
