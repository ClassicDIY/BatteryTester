#!/bin/bash
app='battery.tester'
hostIp=hostname -I | cut -d' ' -f1
docker build -f Dockerfile-pi -t $app .
echo "Build done!"
docker run -p 80:80  -e HostName=hostIp --name 'BatteryTester' $app