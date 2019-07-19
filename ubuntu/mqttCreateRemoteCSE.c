#include <stdio.h>
#include <string.h>
#include "./libok/thingplug.h"

int main()
{
MQTTClient client;

char addr[]		= "tcp://mqtt.sktiot.com:1883";
char id[]		= "lionhr33";
char pw[]		= "RVN6QVpMbjBpNUw2NHB1WDlpY3ZkNWlMVEN1dmJjc1RuLzZkZ3VCUkt6R05SREY2NmZHQkk0TThSRUZXTDg2VQ==";
char deviceId[]	= "lionhr33_20180110_1";
char devicePw[] 	= "123456";
char container[]	= "temperature";
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

printf("Create Remote CSE Success\n");

mqttDisconnect(&client);

return TRUE;
}

