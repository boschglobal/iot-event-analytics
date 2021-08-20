@echo off
set countFile=buildver.txt
(set /P COUNT=<%countFile%)2>nul || set COUNT=0
set /A COUNT+=1
echo:%COUNT%>%countFile%

set buildVersion=v%COUNT%
echo Building %buildVersion%

docker buildx build --progress plain --platform linux/arm64 -t swdceuwedevgsopscr.azurecr.io/iot-event-analytics/iotea-platform-arm64:latest -t swdceuwedevgsopscr.azurecr.io/iot-event-analytics/iotea-platform-arm64:%buildVersion% --load --build-arg HTTP_PROXY=http://host.docker.internal:3128 --build-arg HTTPS_PROXY=http://host.docker.internal:3128 -f docker/platform/Dockerfile.slim.seatadjuster.arm64 .
call az acr login --name swdceuwedevgsopscr
docker push swdceuwedevgsopscr.azurecr.io/iot-event-analytics/iotea-platform-arm64:%buildVersion%
docker push swdceuwedevgsopscr.azurecr.io/iot-event-analytics/iotea-platform-arm64:latest

echo Finished deployment to swdceuwedevgsopscr.azurecr.io/iot-event-analytics/iotea-platform-arm64:latest
echo Finished deployment to swdceuwedevgsopscr.azurecr.io/iot-event-analytics/iotea-platform-arm64:%buildVersion%

