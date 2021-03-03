$app = 'battery.tester'

docker build -f Dockerfile -t $app .
$HostName = hostname.exe 
docker run -p 80:80  -e HostName=$HostName --name 'BatteryTester' $app