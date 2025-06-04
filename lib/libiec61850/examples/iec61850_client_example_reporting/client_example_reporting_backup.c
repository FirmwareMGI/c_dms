/*
 * client_example_reporting.c
 *
 * This example is intended to be used with server_example_basic_io or server_example_goose.
 */

#include "iec61850_client.h"

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>
#include <MQTTClient.h>         
#include <cjson/cJSON.h>


#include "hal_thread.h"

static int running = 1;
#define MAX_REPORTS 5
#define MAX_HOSTS 10

#define ADDRESS     "192.168.2.33"  // Change to your broker address
#define CLIENTID    "IEC61850_ReportClient"
#define TOPIC       "iec61850/report"
#define QOS         1
#define TIMEOUT     10000L

MQTTClient mqttClient;  // Global client handle


typedef struct {
    char dataset[128];
    char rcb[128];
} ReportConfig;

typedef struct {
    char ip[64];
    ReportConfig reports[MAX_REPORTS];
    int reportCount;
    char port[64];
} HostConfig;

typedef struct {
    IedConnection con;
    ClientReportControlBlock rcb;
    IedClientError error;
    ClientDataSet clientDataSet;
    LinkedList dataSetDirectory;
    char* ip;
    int tcpPort;
    const char* dataset;
    const char* rcbName;
    bool connected;
} ReportSession;

ReportSession sessions[100];  // Adjust size accordingly
int sessionCount = 0;

// static char datasetName[100] = "BCUCPLCONTROL1/LLN0$LLN0BRptStDs";
// static char rcbName[100] = "BCUCPLCONTROL1/LLN0$BR$brcbST01";

void sigint_handler(int signalId)
{
    running = 0;
}


// void
// reportCallbackFunction(void* parameter, ClientReport report)
// {
//     LinkedList dataSetDirectory = (LinkedList) parameter;

//     MmsValue* dataSetValues = ClientReport_getDataSetValues(report);

//     // printf("received report for %s with rptId %s\n", ClientReport_getRcbReference(report), ClientReport_getRptId(report));
//     printf("Anjay ada report lur\n");

//     if (ClientReport_hasTimestamp(report)) {
//         time_t unixTime = ClientReport_getTimestamp(report) / 1000;

// #ifdef WIN32
// 		char* timeBuf = ctime(&unixTime);
// #else
// 		char timeBuf[30];
// 		ctime_r(&unixTime, timeBuf);
// #endif

//         // printf("  report contains timestamp (%u): %s", (unsigned int) unixTime, timeBuf);
//     }

//     if (dataSetDirectory) {
//         int i;
//         for (i = 0; i < LinkedList_size(dataSetDirectory); i++) {
//             ReasonForInclusion reason = ClientReport_getReasonForInclusion(report, i);

//             if (reason != IEC61850_REASON_NOT_INCLUDED) {

//                 char valBuffer[500];
//                 sprintf(valBuffer, "no value");

//                 if (dataSetValues) {
//                     MmsValue* value = MmsValue_getElement(dataSetValues, i);

//                     if (value) {
//                         MmsValue_printToBuffer(value, valBuffer, 500);
//                         int arraySize = MmsValue_getArraySize(value);
//                         // printf("arraySize: %d\n", arraySize);
//                         for (int j = 0; j < arraySize; j++) {
//                             MmsValue* arrayValue = MmsValue_getElement(value, j);
//                             if (arrayValue) {
//                                 char arrayBuffer[500];
//                                 MmsValue_printToBuffer(arrayValue, arrayBuffer, 500);
//                                 // printf("array[%d]: %s\n", j, arrayBuffer);
//                             }
//                         }
//                     }
//                 }
//                 LinkedList entry = LinkedList_get(dataSetDirectory, i);
//                 // dataSetDirectory->current = entry;

//                 char* entryName = (char*) entry->data;
//                 printf("entryName: %s\n", entryName);
//                 printf("  %s (included for reason %i): %s\n", entryName, reason, valBuffer);
                
//             }
//         }
//     }
//     // printf("Done report\n");
// }

// #include <MQTTClient.h>  // Include Paho MQTT C client

// extern MQTTClient mqttClient;  // Assume this is initialized and connected somewhere else

void
reportCallbackFunction(void* parameter, ClientReport report)
{
    LinkedList dataSetDirectory = (LinkedList) parameter;

    MmsValue* dataSetValues = ClientReport_getDataSetValues(report);

    printf("Anjay ada report lur\n");

    char timestampBuf[64] = "";
    if (ClientReport_hasTimestamp(report)) {
        time_t unixTime = ClientReport_getTimestamp(report) / 1000;

#ifdef WIN32
        char* timeBuf = ctime(&unixTime);
#else
        char timeBuf[30];
        ctime_r(&unixTime, timeBuf);
#endif
        snprintf(timestampBuf, sizeof(timestampBuf), "%u", (unsigned int) unixTime);
    }
    char entryNameBuffer[100];


    if (dataSetDirectory) {
        char mqttMessage[1024];
        mqttMessage[0] = '\0';  // Clear buffer

        int i;
        for (i = 0; i < LinkedList_size(dataSetDirectory); i++) {
            ReasonForInclusion reason = ClientReport_getReasonForInclusion(report, i);

            if (reason != IEC61850_REASON_NOT_INCLUDED) {

                char valBuffer[500] = "no value";

                if (dataSetValues) {
                    MmsValue* value = MmsValue_getElement(dataSetValues, i);

                    if (value) {
                        MmsValue_printToBuffer(value, valBuffer, sizeof(valBuffer));
                    }
                }

                LinkedList entry = LinkedList_get(dataSetDirectory, i);
                char* entryName = (char*) entry->data;

                // Append info to MQTT message buffer
                char line[600];
                snprintf(line, sizeof(line),
                    "{ \"entry\": \"%s\", \"reason\": %d, \"value\": \"%s\", \"timestamp\": \"%s\" },",
                    entryName, reason, valBuffer, timestampBuf);

                strncat(mqttMessage, line, sizeof(mqttMessage) - strlen(mqttMessage) - 1);

                snprintf(entryNameBuffer, sizeof(entryNameBuffer), "%s", entryName);

                printf("entryName: %s\n", entryName);
                printf("  %s (included for reason %i): %s\n", entryName, reason, valBuffer);
            }
        }

        // Remove last comma if needed
        size_t len = strlen(mqttMessage);
        if (len > 0 && mqttMessage[len - 1] == ',')
            mqttMessage[len - 1] = '\0';

        // Final JSON wrapper
        char finalPayload[1100];
        snprintf(finalPayload, sizeof(finalPayload), "{ \"report\": [ %s ] }", mqttMessage);

        int rc;

        MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;

        // Create client
        rc = MQTTClient_create(&mqttClient, ADDRESS, CLIENTID,
                            MQTTCLIENT_PERSISTENCE_NONE, NULL);
        if (rc != MQTTCLIENT_SUCCESS) {
            printf("Failed to create MQTT client, return code %d\n", rc);
        }

        // Set connection options
        conn_opts.keepAliveInterval = 20;
        conn_opts.cleansession = 1;

        // Connect to the broker
        rc = MQTTClient_connect(mqttClient, &conn_opts);
        if (rc != MQTTCLIENT_SUCCESS) {
            printf("Failed to connect to MQTT broker, return code %d\n", rc);
        }

        printf("Connected to MQTT broker at %s\n", ADDRESS);

        // Publish to MQTT
        MQTTClient_message pubmsg = MQTTClient_message_initializer;
        pubmsg.payload = finalPayload;
        pubmsg.payloadlen = (int) strlen(finalPayload);
        pubmsg.qos = 2;
        pubmsg.retained = 0;

        MQTTClient_deliveryToken token;
        rc = MQTTClient_publishMessage(mqttClient, entryNameBuffer, &pubmsg, &token);
        if (rc == MQTTCLIENT_SUCCESS) {
            MQTTClient_waitForCompletion(mqttClient, token, 1000L);
            printf("MQTT report sent successfully\n");
        } else {
            printf("Failed to publish MQTT report, rc=%d\n", rc);
        }
        MQTTClient_disconnect(mqttClient, 10000);
        MQTTClient_destroy(&mqttClient);
    }
}


int loadHostConfigs(const char* filename, HostConfig* host) {
    FILE* fp = fopen(filename, "r");
    if (!fp) {
        perror("Failed to open config file");
        return 0;
    }

    fseek(fp, 0, SEEK_END);
    long len = ftell(fp);
    rewind(fp);

    char* data = (char*)malloc(len + 1);
    fread(data, 1, len, fp);
    data[len] = '\0';
    fclose(fp);

    cJSON* root = cJSON_Parse(data);
    free(data);
    if (!root) return 0;

    // ✅ Get IP and port
    cJSON* ip = cJSON_GetObjectItem(root, "localIP");
    cJSON* port = cJSON_GetObjectItem(root, "port");
    if (!ip || !port) {
        cJSON_Delete(root);
        return 0;
    }

    strncpy(host->ip, ip->valuestring, sizeof(host->ip));
    strncpy(host->port, port->valuestring, sizeof(host->port));

    // ✅ Get reports
    cJSON* reportList = cJSON_GetObjectItem(root, "reports");
    if (!reportList || !cJSON_IsArray(reportList)) {
        cJSON_Delete(root);
        return 0;
    }

    int reportIndex = 0;
    cJSON* reportEntry;
    cJSON_ArrayForEach(reportEntry, reportList) {
        if (reportIndex >= MAX_REPORTS) break;

        cJSON* dsRef = cJSON_GetObjectItem(reportEntry, "dataSetReference");
        cJSON* rcb = cJSON_GetObjectItem(reportEntry, "rcb");
        cJSON* isEnabled = cJSON_GetObjectItem(reportEntry, "isEnable");

        if (dsRef && rcb && cJSON_IsBool(isEnabled) && cJSON_IsTrue(isEnabled)) {
            strncpy(host->reports[reportIndex].dataset, dsRef->valuestring, sizeof(host->reports[reportIndex].dataset));
            strncpy(host->reports[reportIndex].rcb, rcb->valuestring, sizeof(host->reports[reportIndex].rcb));
            reportIndex++;
        }
    }

    host->reportCount = reportIndex;

    cJSON_Delete(root);
    return 1;  // success
}




int
main(int argc, char** argv)
{
    

    /*
     * Your IEC 61850 client setup and report subscription logic here.
     * Ensure reportCallbackFunction is registered correctly.
     */

    char* configName;
     
    if (argc > 1)
        configName = argv[1];
    printf("Config name: %s\n", configName);
    HostConfig hostConfigs[MAX_HOSTS];
    int numHosts = loadHostConfigs(configName, hostConfigs);
    for (int i = 0; i < numHosts; i++) {
        printf("Connecting to host: %s\n", hostConfigs[i].ip);
        printf("Number of reports: %d\n", hostConfigs[i].reportCount);
    
        for (int j = 0; j < hostConfigs[i].reportCount; j++) {
            printf("  Report %d: Dataset: %s, RCB: %s\n", j + 1,
                hostConfigs[i].reports[j].dataset,
                hostConfigs[i].reports[j].rcb);
    
            IedClientError error;
            IedConnection con = IedConnection_create();
    
            IedConnection_connect(con, &error, hostConfigs[i].ip, 102);
    
            if (error != IED_ERROR_OK) {
                printf("Failed to connect to %s:102\n", hostConfigs[i].ip);
                IedConnection_destroy(con);
                continue;
            }
    
            ClientReportControlBlock rcb = NULL;
            ClientDataSet clientDataSet = NULL;
            LinkedList dataSetDirectory = NULL;
    
            dataSetDirectory = IedConnection_getDataSetDirectory(con, &error, hostConfigs[i].reports[j].dataset, NULL);
            clientDataSet = IedConnection_readDataSetValues(con, &error, hostConfigs[i].reports[j].dataset, NULL);
            rcb = IedConnection_getRCBValues(con, &error, hostConfigs[i].reports[j].rcb, NULL);
    
            if (rcb == NULL || error != IED_ERROR_OK) {
                printf("Failed to get RCB values.\n");
                IedConnection_destroy(con);
                continue;
            }
    
            ClientReportControlBlock_setResv(rcb, true);
            ClientReportControlBlock_setTrgOps(rcb, TRG_OPT_DATA_CHANGED | TRG_OPT_QUALITY_CHANGED | TRG_OPT_GI);
            ClientReportControlBlock_setDataSetReference(rcb, hostConfigs[i].reports[j].dataset);
            ClientReportControlBlock_setRptEna(rcb, true);
            ClientReportControlBlock_setGI(rcb, true);
    
            IedConnection_installReportHandler(con, hostConfigs[i].reports[j].dataset,
                ClientReportControlBlock_getRptId(rcb), reportCallbackFunction, (void*) dataSetDirectory);
    
            IedConnection_setRCBValues(con, &error, rcb, RCB_ELEMENT_RPT_ENA | RCB_ELEMENT_GI, true);
            Thread_sleep(1000);
    
            // Store this session
            sessions[sessionCount].con = con;
            sessions[sessionCount].rcb = rcb;
            sessions[sessionCount].error = error;
            sessions[sessionCount].clientDataSet = clientDataSet;
            sessions[sessionCount].dataSetDirectory = dataSetDirectory;
            sessions[sessionCount].ip = hostConfigs[i].ip;
            sessions[sessionCount].tcpPort = 102;
            sessions[sessionCount].dataset = hostConfigs[i].reports[j].dataset;
            sessions[sessionCount].rcbName = hostConfigs[i].reports[j].rcb;
            sessions[sessionCount].connected = true;
            sessionCount++;
        }
    }
    while (running) {
        for (int i = 0; i < sessionCount; i++) {
            if (!sessions[i].connected){
                printf("Make reconnect here\n");
                IedClientError error;
                IedConnection con = IedConnection_create();
                sessions[i].con = con;
                
                IedConnection_connect(con, &error, hostConfigs[i].ip, 102);
        
                if (error != IED_ERROR_OK) {
                    printf("Failed to connect to %s:102\n", hostConfigs[i].ip);
                    IedConnection_destroy(con);
                    continue;
                }
        
                ClientReportControlBlock_setResv(sessions[i].rcb, true);
                ClientReportControlBlock_setTrgOps(sessions[i].rcb, TRG_OPT_DATA_CHANGED | TRG_OPT_QUALITY_CHANGED | TRG_OPT_GI);
                ClientReportControlBlock_setDataSetReference(sessions[i].rcb, sessions[i].dataset);
                ClientReportControlBlock_setRptEna(sessions[i].rcb, true);
                ClientReportControlBlock_setGI(sessions[i].rcb, true);
        
                IedConnection_installReportHandler(sessions[i].con, sessions[i].dataset,
                    ClientReportControlBlock_getRptId(sessions[i].rcb), reportCallbackFunction, (void*) sessions[i].dataSetDirectory);
                
                    
                IedConnection_setRCBValues(sessions[i].con, &error, sessions[i].rcb, RCB_ELEMENT_RPT_ENA | RCB_ELEMENT_GI, true);
                printf("Nyampe sini bang\n");
                if (error != IED_ERROR_OK) {
                    printf("Failed to connect to %s:102\n", hostConfigs[i].ip);
                    IedConnection_destroy(con);
                    continue;
                }
                sessions[i].connected = true;
                continue;
            }
    
            IedConnectionState state = IedConnection_getState(sessions[i].con);
            if (state != IED_STATE_CONNECTED) {
                printf("Session to %s disconnected.\n", sessions[i].ip);
                sessions[i].connected = false;
            }
            printf("Session to %s is connected.\n", sessions[i].ip);
            printf("Session %d: Dataset: %s, RCB: %s\n", i + 1,
                sessions[i].dataset,
                sessions[i].rcbName);
        }

        Thread_sleep(1000);  // 1 second
    }


    for (int i = 0; i < sessionCount; i++) {
        if (sessions[i].rcb) {
            ClientReportControlBlock_setRptEna(sessions[i].rcb, false);
            IedConnection_setRCBValues(sessions[i].con, NULL, sessions[i].rcb, RCB_ELEMENT_RPT_ENA, true);
        }
    
        if (sessions[i].con) IedConnection_close(sessions[i].con);
        if (sessions[i].clientDataSet) ClientDataSet_destroy(sessions[i].clientDataSet);
        if (sessions[i].rcb) ClientReportControlBlock_destroy(sessions[i].rcb);
        if (sessions[i].dataSetDirectory) LinkedList_destroy(sessions[i].dataSetDirectory);
        if (sessions[i].con) IedConnection_destroy(sessions[i].con);
    }

    
}


