#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "MQTTClient.h"

#define TRUE			1
#define FALSE			0
#define APP_EUI			"thingplug"
#define NOTIFY_SUB_NAME 	"myNotification"
#define QOS			1
#define TIMEOUT			10000L
#define BUF_SIZE		128

// The pathes will be created dynamically
char mqttPubTopic[BUF_SIZE]	= "";
char mqttSubTopic[BUF_SIZE]	= "";
char mqttPubPath[BUF_SIZE]	= "";
char mqttRemoteCSE[BUF_SIZE]	= "";
char mqttContainer[BUF_SIZE]	= "";
char mqttLatest[BUF_SIZE]	= "";
char mqttSubscription[BUF_SIZE] = "";

const char frameMqttPubTopic[]	= "/oneM2M/req_msg/+/%s";	// deviceId
const char frameMqttSubTopic[]	= "/oneM2M/resp/%s/+";		// deviceId
const char frameMqttPubPath[]	= "/oneM2M/req/%s/%s";		// deviceId, appEUI
const char frameMqttRemoteCSE[]	= "/%s/v1_0/remoteCSE-%s";	// appEUI, deviceId
const char frameMqttContainer[] = "%s/container-%s";		// remoteCSE, container
const char frameMqttLatest[]	= "%s/latest";			//container
const char frameMqttSubscription[] = "%s/subscription-%s";

char bufRequest[1024]		= "";
char strNL[BUF_SIZE]		= "";
char strExt[BUF_SIZE]		= "";
char strDkey[BUF_SIZE]		= "";
char dataName[BUF_SIZE] 	= "";
char dataValue[BUF_SIZE] 	= "";
void (*mqttCallback)(char*)	= NULL;


char address[BUF_SIZE]  	= "";
char userName[BUF_SIZE] 	= "";
char passWord[BUF_SIZE] 	= "";
char deviceId[BUF_SIZE] 	= "";
char passCode[BUF_SIZE] 	= "";
char container[BUF_SIZE] 	= "";
char targetDeviceId[BUF_SIZE]   = "";

// Functions
void generateRi(char * buf);
int parseValue(char* buf, const char * payload, const int len, const char * param);
void printResultCode(char * buf);
int callbackArrived(void *context, char *topicName, int topicLen, MQTTClient_message *message);

// MQTT Process Steps
enum MqttStep {
	CREATE_NODE,
	CREATE_NODE_REQUESTED,
	CREATE_REMOTE_CSE,
	CREATE_REMOTE_CSE_REQUESTED,
	CREATE_CONTAINER,
	CREATE_CONTAINER_REQUESTED,
	CREATE_MGMT_CMD,
	CREATE_MGMT_CMD_REQUESTED,
	SUBSCRIBE,
	SUBSCRIBE_REQUESTED,
	CREATE_CONTENT_INSTANCE,
	CREATE_CONTENT_INSTANCE_REQUESTED,
	GET_LATEST,
	GET_LATEST_REQUESTED,
	DELETE_SUBSCRIBE,
	DELETE_SUBSCRIBE_REQUESTED,
	FINISH
};
enum MqttStep step = CREATE_NODE;

// step 1 - params: appEUI, deviceId, ri, deviceId, deviceId, deviceId
const char frameCreateNode[] =
"<m2m:req>\
<op>1</op>\
<ty>14</ty>\
<to>/%s/v1_0</to>\
<fr>%s</fr>\
<ri>%s</ri>\
<cty>application/vnd.onem2m-prsp+xml</cty>\
<nm>%s</nm>\
<pc>\
<nod>\
<ni>%s</ni>\
<mga>MQTT|%s</mga>\
</nod>\
</pc>\
</m2m:req>";

// step 2 – params: AppEUI, deviceId, ri, passCode, deviceId, deviceId, nl
const char frameCreateRemoteCSE[] = 
"<m2m:req>\
<op>1</op>\
<ty>16</ty>\
<to>/%s/v1_0</to>\
<fr>%s</fr>\
<ri>%s</ri>\
<passCode>%s</passCode>\
<cty>application/vnd.onem2m-prsp+xml</cty>\
<nm>%s</nm>\
<pc>\
<csr>\
<cst>3</cst>\
<csi>%s</csi>\
<rr>true</rr>\
<nl>%s</nl>\
</csr>\
</pc>\
</m2m:req>";

// step 3 – params: cse, deviceId, ri, container, dKey
const char frameCreateContainer[] = 
"<m2m:req>\
<op>1</op>\
<ty>3</ty>\
<to>%s</to>\
<fr>%s</fr>\
<ri>%s</ri>\
<cty>application/vnd.onem2m-prsp+xml</cty>\
<nm>%s</nm>\
<dKey>%s</dKey>\
<pc>\
<cnt>\
<lbl>con</lbl>\
</cnt>\
</pc>\
</m2m:req>";

// step 4 – params: appEUI, deviceId, ri, deviceId, dkey, strExt
const char frameCreateMgmtCmd[] = 
"<m2m:req>\
<op>1</op>\
<ty>12</ty>\
<to>/%s/v1_0</to>\
<fr>%s</fr>\
<ri>%s</ri>\
<cty>application/vnd.onem2m-prsp+xml</cty>\
<nm>%s_turnOn</nm>\
<dKey>%s</dKey>\
<pc>\
<mgc>\
<cmt>turnOn</cmt>\
<exe>false</exe>\
<ext>%s</ext>\
</mgc>\
</pc>\
</m2m:req>";

// step 4-1 - params: mqttContainer, deviceId, strRi, notificationName, uKey, targetdeviceId
const char frameCreateSubscribe[] =
"<m2m:req>\
<op>1</op>\
<to>%s</to>\
<fr>%s</fr>\
<ty>23</ty>\
<ri>%s</ri>\
<nm>%s</nm>\
<uKey>%s</uKey>\
<cty>application/vnd.onem2m-prsp+xml</cty>\
<pc>\
<sub>\
<enc>\
<rss>1</rss>\
</enc>\
<nu>MQTT|%s</nu>\
<nct>2</nct>\
</sub>\
</pc>\
</m2m:req>";

// step 5 - params: container, deviceId, ri, dkey, name, value
const char frameCreateContentInstance[] = 
"<m2m:req>\
<op>1</op>\
<ty>4</ty>\
<to>%s</to>\
<fr>%s</fr>\
<ri>%s</ri>\
<cty>application/vnd.onem2m-prsp+xml</cty>\
<dKey>%s</dKey>\
<pc>\
<cin>\
<cnf>%s</cnf>\
<con>%s</con>\
</cin>\
</pc>\
</m2m:req>";

// step 6 – mqttLatest, deviceId, Ri, uKey
const char frameCreateLatest[] = 
"<m2m:req>\
<op>2</op>\
<to>%s</to>\
<fr>%s</fr>\
<ri>%s</ri>\
<cty>application/vnd.onem2m-prsp+xml</cty>\
<uKey>%s</uKey>\
</m2m:req>";

// step 7 - params: container, deviceId, ri, dKey, deviceId 
const char frameDeleteSubscribe[] = 
"<m2m:req>\
<op>4</op>\
<to>%s</to>\
<fr>%s</fr>\
<ri>%s</ri>\
<uKey>%s</uKey>\
<cty>application/vnd.onem2m-prsp+xml</cty>\
<pc>\
<sub>\
<enc>\
<rss>1</rss>\
</enc>\
<nu>MQTT|%s</nu>\
<nct>2</nct>\
</sub>\
</pc>\
</m2m:req>";



int mqttConnect(MQTTClient * client, char* addr, char* id, char* pw, char* devId) 
{
    strcpy(address, addr);
    strcpy(userName, id);
    strcpy(passWord, pw);
    strcpy(deviceId, devId);

    MQTTClient_create(client, addr, deviceId, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    int rc;

    // add ID, PW
    conn_opts.username= userName;
    conn_opts.password= passWord;

    sprintf(mqttPubTopic, frameMqttPubTopic, deviceId);
    sprintf(mqttSubTopic, frameMqttSubTopic, deviceId);
    sprintf(mqttPubPath, frameMqttPubPath, deviceId, APP_EUI);
    sprintf(mqttRemoteCSE, frameMqttRemoteCSE, APP_EUI, deviceId);

    // set callback functions
    rc = MQTTClient_setCallbacks(*client, NULL, NULL, callbackArrived, NULL);
    if(rc==MQTTCLIENT_FAILURE) 
    {
	printf("Set Callback Failed\n");
	return FALSE;
    }

    printf("Attempting MQTT connection...\n");
    rc = MQTTClient_connect(*client, &conn_opts);
    if (rc == MQTTCLIENT_SUCCESS)
    {
	printf("Mqtt connected\n");

	// registration of the topics
	int rc = MQTTClient_subscribe(*client, mqttPubTopic, QOS);
	if(rc==MQTTCLIENT_SUCCESS) printf("Subscription of Pub Topic Success!!\n");
	else {
		printf("Pub Topic Subscription failed!!\n");
		return FALSE;
	}

	rc = MQTTClient_subscribe(*client, mqttSubTopic, QOS);
	if(rc==MQTTCLIENT_SUCCESS) printf("Subscription of Sub Topic Success!!\n\n");
	else {
		printf("Sub Topic Subscription failed!!\n");
		return FALSE;
	}
    }
    else 
    {
        printf("Failed to connect, return code %d\n", rc);
	return FALSE;
    }

    return TRUE;
}

void mqttDisconnect(MQTTClient * client)
{
    MQTTClient_disconnect(*client, TIMEOUT);
    MQTTClient_destroy(client);
}

int callbackArrived(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
  printf("MQTT Message arrived: Topic=%s\n", topicName);
  char * payload = (char*)message->payload;
  int len = message->payloadlen;

  if(len>0) payload[len]='\0';
  printf("Payload(%d): %s\n", len, payload);

  // error check
  char strRsc[128]  = "";
  int rc= parseValue(strRsc, payload, len, "rsc");
  if(rc== MQTTCLIENT_SUCCESS ) 
  {
    printResultCode(strRsc);
    int resultCode = atoi(strRsc);
    if(resultCode == 4004) return FALSE;
    if(resultCode == 4000) return FALSE;
  
    char strRi[128] = "";
    generateRi(strRi);

    switch(step) {
	case CREATE_NODE_REQUESTED:
    		// parse response message
    		rc = parseValue(strNL, strstr(payload, "pc"), len, "ri");
    		if(rc==MQTTCLIENT_SUCCESS) {
			printf("ri:%s\n", strNL);
			strcpy(strExt, strNL);
			step = CREATE_REMOTE_CSE;
		}
		break;

	case  CREATE_REMOTE_CSE_REQUESTED : 
		rc = parseValue(strDkey, payload, len, "dKey");
		printf("dKey=%s\n", strDkey);
		if(rc==MQTTCLIENT_SUCCESS) {
			step = CREATE_CONTAINER;
      		}
      		break;

	case CREATE_CONTAINER_REQUESTED:
		step = CREATE_MGMT_CMD;
		break;

	case CREATE_MGMT_CMD_REQUESTED:
		step = SUBSCRIBE;
		break;

	case SUBSCRIBE_REQUESTED:
		step = CREATE_CONTENT_INSTANCE;
		break;
	case CREATE_CONTENT_INSTANCE_REQUESTED:
		step = DELETE_SUBSCRIBE;
		break;

	default:
		step = FINISH;
		break;
    }
  }
  else
  {
    // Notification from ThingPlug Server
    char strCon[128] = "";
    rc = parseValue(strCon, payload, len, "con");
    if(rc==MQTTCLIENT_SUCCESS && mqttCallback!=NULL) mqttCallback(strCon);
  } 

  MQTTClient_freeMessage(&message);
  MQTTClient_free(topicName);

  return 1; // Do Not Need to be recalled.
}

// generates a unique resource ID
void generateRi(char * buf)
{
	if(buf==NULL) return;
	sprintf(buf, "%s_%lu", deviceId, time(NULL));
}

int parseValue(char* buf, const char * payload, const int len, const char * param)
{
  if(payload==NULL)
  {
	printf("parseValue error: Payload is NULL\n");
	return MQTTCLIENT_FAILURE;
  } 

  int result = MQTTCLIENT_FAILURE;
  int lenParam = strlen(param);
  
  char tagBegin[128]="";
  sprintf(tagBegin, "<%s>", param);
  
  char tagEnd[128];
  sprintf(tagEnd, "</%s>", param);

  char * pBegin = strstr(payload, tagBegin);
  if(pBegin==NULL) return result;
  int indexBegin = pBegin - payload;
  if(indexBegin>0){
     char* pEnd = strstr(payload, tagEnd);
     int indexEnd = pEnd - payload;
     int indexValue = indexBegin+lenParam+2;
     int lenValue = indexEnd-indexValue;
     strncpy(buf, &payload[indexValue], lenValue);
     buf[lenValue]='\0';
     result = MQTTCLIENT_SUCCESS;
  }

  return result;
}

void printResultCode(char * buf) 
{
  if(buf==NULL) return;
  printf("[result code]: ");
  
  int resultCode = atoi(buf);
  switch(resultCode) {
    case 2000: printf("OK\n"); break;
    case 2001: printf("CREATED\n"); break;
    case 2002: printf("DELETED\n"); break;
    case 2004: printf("UPDATED\n"); break;
    case 2100: printf("CONTENT_EMPTY\n"); break;
    case 4105: printf("EXIST\n"); break;
    case 4004: printf("NOT FOUND\n"); break;
    default: printf("UNKNOWN ERROR\n"); break;
  }
}

int mqttCreateNode(MQTTClient * client, char * devPw)
{
    step = CREATE_NODE_REQUESTED;
    strcpy(passCode, devPw);

    MQTTClient_deliveryToken token;
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    int rc = 0;

    char ri[100]="";
    generateRi(ri);

    sprintf(bufRequest,frameCreateNode, APP_EUI, deviceId, ri, deviceId, deviceId, deviceId);    
    printf("1. Create Node :\n payload=%s\n", bufRequest);

    // publish bufRequest
    pubmsg.payload 	= bufRequest;
    pubmsg.payloadlen 	= strlen(bufRequest);
    pubmsg.qos 		= QOS;
    pubmsg.retained 	= 0;
    MQTTClient_publishMessage(*client, mqttPubPath, &pubmsg, &token);

    printf("Waiting for publication\n");
    rc = MQTTClient_waitForCompletion(*client, token, TIMEOUT);
    if(rc!=MQTTCLIENT_SUCCESS) {
	printf("Create Node Failed\n\n"); 
	return FALSE;
    }

    printf("Create Node Success\n\n");

    return TRUE;
}

int mqttCreateRemoteCSE(MQTTClient * client)
{
    step = CREATE_REMOTE_CSE_REQUESTED;

    MQTTClient_deliveryToken token;
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    int rc = 0;

    char strRi[100]="";
    generateRi(strRi);

    sprintf(bufRequest, frameCreateRemoteCSE, APP_EUI,
            deviceId, strRi, passCode, deviceId, deviceId, strNL);

    printf("2. Create RemoteCSE :\n payload=%s\n", bufRequest);
    pubmsg.payload 	= bufRequest;
    pubmsg.payloadlen 	= strlen(bufRequest);
    pubmsg.qos 		= QOS;
    pubmsg.retained 	= 0;
    MQTTClient_publishMessage(*client, mqttPubPath, &pubmsg, &token);

    printf("Waiting for publication\n");
    rc = MQTTClient_waitForCompletion(*client, token, TIMEOUT);
    if(rc!=MQTTCLIENT_SUCCESS) {
	printf("Publish Failed\n\n"); 
	return FALSE;
    }

    printf("Publish Success\n\n");

    return TRUE;
}


int mqttCreateContainer(MQTTClient * client, char* con)
{
    step = CREATE_CONTAINER_REQUESTED;
    strcpy(container, con);

    MQTTClient_deliveryToken token;
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    int rc = 0;

    char strRi[100]="";
    generateRi(strRi);

    sprintf(mqttContainer, frameMqttContainer, mqttRemoteCSE, container);
    sprintf( bufRequest, frameCreateContainer,
             mqttRemoteCSE, deviceId, strRi, container, strDkey);

    printf("3. Create Container :\n payload=%s\n", bufRequest);
    pubmsg.payload 	= bufRequest;
    pubmsg.payloadlen 	= strlen(bufRequest);
    pubmsg.qos 		= QOS;
    pubmsg.retained 	= 0;
    MQTTClient_publishMessage(*client, mqttPubPath, &pubmsg, &token);

    printf("Waiting for publication\n");
    rc = MQTTClient_waitForCompletion(*client, token, TIMEOUT);
    if(rc!=MQTTCLIENT_SUCCESS) {
	printf("Publish Failed\n\n"); 
	return FALSE;
    }

    printf("Publish Success\n\n");

    return TRUE;
}

int mqttCreateMgmtCmd(MQTTClient * client)
{
    step = CREATE_MGMT_CMD_REQUESTED;

    MQTTClient_deliveryToken token;
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    int rc = 0;

    char strRi[100]="";
    generateRi(strRi);

    sprintf( bufRequest, frameCreateMgmtCmd,
        APP_EUI, deviceId, strRi, deviceId, strDkey, strExt);


    printf("4. Create Mgmt Cmd :\n payload=%s\n", bufRequest);
    pubmsg.payload 	= bufRequest;
    pubmsg.payloadlen 	= strlen(bufRequest);
    pubmsg.qos 		= QOS;
    pubmsg.retained 	= 0;
    MQTTClient_publishMessage(*client, mqttPubPath, &pubmsg, &token);

    printf("Waiting for publication\n");
    rc = MQTTClient_waitForCompletion(*client, token, TIMEOUT);
    if(rc!=MQTTCLIENT_SUCCESS) {
    	printf("Publish Failed\n\n"); 
	return FALSE;
    }

    printf("Publish Success\n\n");

    return TRUE;
}

int mqttCreateContentInstance(MQTTClient * client, char * dataValue)
{
    step = CREATE_CONTENT_INSTANCE_REQUESTED;

    MQTTClient_deliveryToken token;
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    int rc = 0;

    char strRi[128] = "";
    generateRi(strRi);

    strcpy(dataName, "text");	// data type
    sprintf(mqttContainer, frameMqttContainer, mqttRemoteCSE, container);

    sprintf(bufRequest, frameCreateContentInstance, 
	mqttContainer, deviceId, strRi, strDkey, dataName, dataValue);

    printf("5. Create Content Instance :\n payload=%s\n", bufRequest);
    pubmsg.payload 	= bufRequest;
    pubmsg.payloadlen 	= strlen(bufRequest);
    pubmsg.qos 		= QOS;
    pubmsg.retained 	= 0;
    MQTTClient_publishMessage(*client, mqttPubPath, &pubmsg, &token);

    printf("Waiting for publication\n");
    rc = MQTTClient_waitForCompletion(*client, token, TIMEOUT);
    if(rc!=MQTTCLIENT_SUCCESS) {
	printf("Publish Failed\n\n"); 
	return FALSE;
    }

    printf("Publish Success\n\n");
    return TRUE;
}
int mqttCreateLatest(MQTTClient * client )
{
    step = GET_LATEST_REQUESTED;
    MQTTClient_deliveryToken token;
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    int rc = 0;

    char strRi[100]="";
    generateRi(strRi);

    sprintf(mqttLatest, frameMqttLatest,mqttContainer);
    sprintf( bufRequest, frameCreateLatest,
        mqttLatest, deviceId, strRi, passWord);


    printf("6. Create Latest :\n payload=%s\n", bufRequest);
    pubmsg.payload 	= bufRequest;
    pubmsg.payloadlen 	= strlen(bufRequest);
    pubmsg.qos 		= QOS;
    pubmsg.retained 	= 0;
    MQTTClient_publishMessage(*client, mqttPubPath, &pubmsg, &token);

    printf("Waiting for publication\n");
    rc = MQTTClient_waitForCompletion(*client, token, TIMEOUT);
    if(rc!=MQTTCLIENT_SUCCESS) {
    	printf("Publish Failed\n\n"); 
	return FALSE;
    }

    printf("Publish Success\n\n");

    return TRUE;
}





int mqttSubscribe(MQTTClient * client,char * targetId,  void (*fp)(char *))
{
    step = SUBSCRIBE_REQUESTED;

    mqttCallback = fp;

    strcpy(targetDeviceId, targetId);
    MQTTClient_deliveryToken token;
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    int rc = 0;

    char strRi[128] = "";
    generateRi(strRi);

    sprintf(mqttRemoteCSE, frameMqttRemoteCSE, APP_EUI, targetDeviceId);
    sprintf(mqttContainer, frameMqttContainer, mqttRemoteCSE, container);

    sprintf( bufRequest, frameCreateSubscribe,
             mqttContainer, deviceId, strRi, NOTIFY_SUB_NAME, passWord, deviceId);

    printf("4-1. Subscribe :\n payload=%s\n", bufRequest);
    pubmsg.payload 	= bufRequest;
    pubmsg.payloadlen 	= strlen(bufRequest);
    pubmsg.qos 		= QOS;
    pubmsg.retained 	= 0;
    MQTTClient_publishMessage(*client, mqttPubPath, &pubmsg, &token);

    printf("Waiting for publication\n");
    rc = MQTTClient_waitForCompletion(*client, token, TIMEOUT);
    if(rc!=MQTTCLIENT_SUCCESS) {
    	printf("Publish Failed\n\n"); 
	return FALSE;
    }

    printf("Publish Success\n\n");

    return TRUE;
}
int mqttDeleteSubscribe(MQTTClient * client, char * targetDeviceId)
{
    step = DELETE_SUBSCRIBE_REQUESTED;

    MQTTClient_deliveryToken token;
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    int rc = 0;

    char strRi[128] = "";
    generateRi(strRi);

    sprintf(mqttRemoteCSE, frameMqttRemoteCSE, APP_EUI, targetDeviceId);
    sprintf(mqttContainer, frameMqttContainer, mqttRemoteCSE, container);
    sprintf(mqttSubscription, frameMqttSubscription, mqttContainer, NOTIFY_SUB_NAME);
    sprintf(bufRequest, frameDeleteSubscribe, mqttSubscription,
	deviceId, strRi, passWord, deviceId);

    printf("4-2. Delete Subscribe :\n payload=%s\n", bufRequest);
    pubmsg.payload 	= bufRequest;
    pubmsg.payloadlen 	= strlen(bufRequest);
    pubmsg.qos 		= QOS;
    pubmsg.retained 	= 0;
    MQTTClient_publishMessage(*client, mqttPubPath, &pubmsg, &token);

    printf("Waiting for publication\n");
    rc = MQTTClient_waitForCompletion(*client, token, TIMEOUT);
    if(rc!=MQTTCLIENT_SUCCESS) {
    	printf("Publish Failed\n\n"); 
	return FALSE;
    }

    printf("Publish Success\n\n");

    return TRUE;
}
