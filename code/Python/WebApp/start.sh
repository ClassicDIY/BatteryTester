#!/bin/bash
app='battery.tester'
hostIp=$(hostname -I | cut -d' ' -f1)
docker container stop 'BatteryTester'
docker container prune -f
docker build -f Dockerfile -t $app .
echo "Build done!"
docker run -p 80:5000  -e HostName=$hostIp --name 'BatteryTester' $app