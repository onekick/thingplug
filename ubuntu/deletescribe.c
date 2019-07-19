#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "./libok/thingplug.h"
//#include "./libok/temperature.h"


void myFunction(char * strCon)
{
	printf("notificaion arrived : %s\n",strCon);
}
int main()
{
MQTTClient client;

char addr[]		= "tcp://mqtt.sktiot.com:1883";
char id[]		= "lionhr33";
char pw[]		= "RVN6QVpMbjBpNUw2NHB1WDlpY3ZkNWlMVEN1dmJjc1RuLzZkZ3VCUkt6R05SREY2NmZHQkk0TThSRUZXTDg2VQ==";
char deviceId[]	= "lionhr33_arduinoADK";
char devicePw[] 	= "123456";
char container[]	= "temperature";
char targetDeviceId[] = "lionhr33_20180110_1";
if(!mqttConnect(&client, addr, id, pw, deviceId)) {
printf("1. mqtt connect failed\n");
return FALSE;
}

if(!mqttCreateNode(&client, devicePw)) {
printf("2. mqtt create node failed\n");
return FALSE;
}

if(!mqttCreateRemoteCSE(&client)) {
printf("3. mqtt create remote cse failed\n");
return FALSE;
}
if(!mqttCreateContainer(&client, container)) {
printf("4. mqtt create container failed\n");
return FALSE;
}
if(!mqttCreateMgmtCmd(&client)) {
printf("5. mqtt create mgmt cmd failed\n");
return FALSE;
}

if(!mqttSubscribe(&client, targetDeviceId,  myFunction)){
printf("7. mqtt subscribe failed\n");
return FALSE;
}

for(int i = 0; i<20; i++)
{
sleep(1);
}
if(!mqttDeleteSubscribe(&client, targetDeviceId)){
printf("8. mqtt delete subscribe failed\n");
return FALSE;
}
mqttDisconnect(&client);

return TRUE;
}

