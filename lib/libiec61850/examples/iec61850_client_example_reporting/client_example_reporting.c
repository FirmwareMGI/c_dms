/*
 * client_example_reporting.c
 *
 * Example client for IEC 61850 report handling and MQTT publishing.
 * Works with `server_example_basic_io` or `server_example_goose`.
 */

#include "iec61850_client.h"
#include "hal_thread.h"
#include <MQTTClient.h>
#include <cjson/cJSON.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <signal.h>

#define MAX_REPORTS 5
#define MAX_HOSTS 10
#define MAX_SESSIONS 100

#define MQTT_ADDRESS     "tcp://192.168.2.33:1883"
#define MQTT_CLIENTID    "IEC61850_ReportClient"
#define MQTT_QOS         2
#define MQTT_TIMEOUT     10000L
MQTTClient mqttClient;


static int running = 1;

char machine_code_[64] = "";

typedef struct {
    char dataset[128];
    char rcb[128];
} ReportConfig;

typedef struct {
    char ip[64];
    char port[64];
    ReportConfig reports[MAX_REPORTS];
    int reportCount;
    char machineCode[64];  // Optional field for machine code
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

ReportSession sessions[MAX_SESSIONS];
int sessionCount = 0;

/* === Signal Handling === */
void sigint_handler(int signalId)
{
    running = 0;
}

void cleanupMqttClient()
{
    MQTTClient_disconnect(mqttClient, MQTT_TIMEOUT);
    MQTTClient_destroy(&mqttClient);
}

int initMqttClient()
{
    int rc;

    rc = MQTTClient_create(&mqttClient, MQTT_ADDRESS, MQTT_CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    if (rc != MQTTCLIENT_SUCCESS) {
        printf("MQTT client creation failed: %d\n", rc);
        return rc;
    }

    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    conn_opts.keepAliveInterval = 5;
    conn_opts.cleansession = 1;

    rc = MQTTClient_connect(mqttClient, &conn_opts);
    if (rc != MQTTCLIENT_SUCCESS) {
        printf("MQTT connect failed: %d\n", rc);
        MQTTClient_destroy(&mqttClient);
        return rc;
    }

    return MQTTCLIENT_SUCCESS;
}


/* === MQTT Report Callback === */
void reportCallbackFunction(void* parameter, ClientReport report)
{
    LinkedList dataSetDirectory = (LinkedList) parameter;
    MmsValue* dataSetValues = ClientReport_getDataSetValues(report);
    char timestampBuf[64] = "";

    // Get current system time
    time_t now = time(NULL);
    struct tm *timeinfo = localtime(&now); // Use gmtime(&now) if you prefer UTC

    if (timeinfo != NULL) {
        strftime(timestampBuf, sizeof(timestampBuf), "%Y-%m-%d %H:%M:%S", timeinfo);
    } else {
        strncpy(timestampBuf, "Invalid time", sizeof(timestampBuf));
    }


    char mqttMessage[1024] = "";
    char entryNameBuffer[100] = "";

    for (int i = 0; i < LinkedList_size(dataSetDirectory); i++) {
        ReasonForInclusion reason = ClientReport_getReasonForInclusion(report, i);

        if (reason == IEC61850_REASON_NOT_INCLUDED)
            continue;

        char valBuffer[500] = "no value";
        MmsValue* value = MmsValue_getElement(dataSetValues, i);
        if (value) MmsValue_printToBuffer(value, valBuffer, sizeof(valBuffer));

        LinkedList entry = LinkedList_get(dataSetDirectory, i);
        char* entryName = (char*) entry->data;

        snprintf(entryNameBuffer, sizeof(entryNameBuffer), "%s", entryName);
        char firstValue[100] = "unknown";

        // Find the opening brace
        char* start = strchr(valBuffer, '{');
        if (start) {
            // Move past the opening brace
            start++;
            
            // Find the comma that ends the first value
            char* end = strchr(start, ',');
            if (end) {
                // Copy the first value into firstValue buffer
                size_t len = end - start;
                if (len < sizeof(firstValue)) {
                    strncpy(firstValue, start, len);
                    firstValue[len] = '\0'; // Null-terminate
                }
            }
        }

        printf("First value: %s\n", firstValue);

        char line[600];
        snprintf(line, sizeof(line),
        "{value: %s, \"timestamp\": \"%s\" }", valBuffer, timestampBuf);

        printf("entry: %s, reason: %d, value: %s\n", entryName, reason, valBuffer);
        printf("Formatted timestamp: %s\n", timestampBuf);

        MQTTClient_message pubmsg = MQTTClient_message_initializer;
        pubmsg.payload = line;
        pubmsg.payloadlen = (int)strlen(line);
        pubmsg.qos = MQTT_QOS;
        pubmsg.retained = 0;
        char topic[150];
        // printf("MachineCode: %s\n",);
        snprintf(topic, sizeof(topic), "DMS/%s/IEC61850/Reports/%s", machine_code_, entryNameBuffer);



        MQTTClient_deliveryToken token;
        int rc = MQTTClient_publishMessage(mqttClient, topic, &pubmsg, &token);
        if (rc == MQTTCLIENT_SUCCESS) {
            MQTTClient_waitForCompletion(mqttClient, token, MQTT_TIMEOUT);
            printf("MQTT report published successfully.\n"); //sads
        } else {
            printf("MQTT publish failed: %d\n", rc);
        }
    }
}

/* === Config Loader === */
int loadHostConfigs(const char* filename, HostConfig* host)
{
    FILE* fp = fopen(filename, "r");
    if (!fp) {
        perror("Config file open failed");
        return 0;
    }

    fseek(fp, 0, SEEK_END);
    long len = ftell(fp);
    rewind(fp);

    char* data = malloc(len + 1);
    fread(data, 1, len, fp);
    data[len] = '\0';
    fclose(fp);

    cJSON* root = cJSON_Parse(data);
    free(data);
    if (!root) return 0;

    cJSON* ip = cJSON_GetObjectItem(root, "localIP");
    cJSON* port = cJSON_GetObjectItem(root, "port");
    cJSON* machine_code = cJSON_GetObjectItem(root, "machineCode");
    if (!ip || !port) {
        cJSON_Delete(root);
        return 0;
    }

    strncpy(host->ip, ip->valuestring, sizeof(host->ip));
    strncpy(host->port, port->valuestring, sizeof(host->port));
    if (machine_code) {
        strncpy(host->machineCode, machine_code->valuestring, sizeof(host->machineCode));
    } else {
        host->machineCode[0] = '\0'; // Default to empty if not provided
    }

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

        if (dsRef && rcb && cJSON_IsTrue(isEnabled)) {
            strncpy(host->reports[reportIndex].dataset, dsRef->valuestring, sizeof(host->reports[reportIndex].dataset));
            strncpy(host->reports[reportIndex].rcb, rcb->valuestring, sizeof(host->reports[reportIndex].rcb));
            reportIndex++;
        }
    }

    host->reportCount = reportIndex;
    cJSON_Delete(root);
    return 1;
}

/* === Main Logic === */
int main(int argc, char** argv)
{
    signal(SIGINT, sigint_handler);

    if (argc < 2) {
        printf("Usage: %s <config_file.json>\n", argv[0]);
        return 1;
    }

    const char* configFile = argv[1];
    HostConfig hostConfig;
    initMqttClient();

    if (!loadHostConfigs(configFile, &hostConfig)) {
        printf("Failed to load host configuration.\n");
        return 1;
    }
    strncpy(machine_code_, hostConfig.machineCode, sizeof(machine_code_));

    
    for (int j = 0; j < hostConfig.reportCount; j++) {
        printf("Connecting to %s:%s - Dataset: %s, RCB: %s\n",
               hostConfig.ip, hostConfig.port,
               hostConfig.reports[j].dataset,
               hostConfig.reports[j].rcb);


        IedClientError error;
        IedConnection con = IedConnection_create();
        IedConnection_connect(con, &error, hostConfig.ip, 102);

        if (error != IED_ERROR_OK) {
            printf("Connection failed: %s\n", hostConfig.ip);
            IedConnection_destroy(con);
            continue;
        }

        LinkedList dataSetDirectory = IedConnection_getDataSetDirectory(con, &error, hostConfig.reports[j].dataset, NULL);
        ClientDataSet dataSet = IedConnection_readDataSetValues(con, &error, hostConfig.reports[j].dataset, NULL);
        ClientReportControlBlock rcb = IedConnection_getRCBValues(con, &error, hostConfig.reports[j].rcb, NULL);

        if (!rcb || error != IED_ERROR_OK) {
            printf("Failed to get RCB values.\n");
            IedConnection_destroy(con);
            continue;
        }

        ClientReportControlBlock_setResv(rcb, true);
        ClientReportControlBlock_setTrgOps(rcb, TRG_OPT_DATA_CHANGED | TRG_OPT_QUALITY_CHANGED | TRG_OPT_GI);
        ClientReportControlBlock_setDataSetReference(rcb, hostConfig.reports[j].dataset);
        ClientReportControlBlock_setRptEna(rcb, true);
        ClientReportControlBlock_setGI(rcb, true);

        IedConnection_installReportHandler(con, hostConfig.reports[j].dataset,
            ClientReportControlBlock_getRptId(rcb), reportCallbackFunction, dataSetDirectory);

        IedConnection_setRCBValues(con, &error, rcb, RCB_ELEMENT_RPT_ENA | RCB_ELEMENT_GI, true);

        sessions[sessionCount++] = (ReportSession){
            .con = con,
            .rcb = rcb,
            .error = error,
            .clientDataSet = dataSet,
            .dataSetDirectory = dataSetDirectory,
            .ip = hostConfig.ip,
            .tcpPort = 102,
            .dataset = hostConfig.reports[j].dataset,
            .rcbName = hostConfig.reports[j].rcb,
            .connected = true
        };
    }

    // while (running) {
    //     for (int i = 0; i < sessionCount; i++) {
    //         IedConnectionState state = IedConnection_getState(sessions[i].con);

    //         if (state != IED_STATE_CONNECTED) {
    //             printf("Disconnected from %s. Will attempt to reconnect.\n", sessions[i].ip);
    //             sessions[i].connected = false;
    //         } else {
    //             printf("Connected to %s - Dataset: %s, RCB: %s\n",
    //                 sessions[i].ip, sessions[i].dataset, sessions[i].rcbName);
    //         }
    //     }
    //     Thread_sleep(1000);
    // }
    while (running) {
        for (int i = 0; i < sessionCount; i++) {
            if (!sessions[i].connected) {
                printf("Reconnecting to %s...\n", sessions[i].ip);

                IedClientError error;
                IedConnection con = IedConnection_create();
                sessions[i].con = con;

                IedConnection_connect(con, &error, sessions[i].ip, sessions[i].tcpPort);

                if (error != IED_ERROR_OK) {
                    printf("Failed to connect to %s:%d (error: %d)\n", sessions[i].ip, sessions[i].tcpPort, error);
                    IedConnection_destroy(con);
                    sessions[i].connected = false;
                    continue;
                }

                sessions[i].error = error;

                ClientReportControlBlock_setResv(sessions[i].rcb, true);
                ClientReportControlBlock_setTrgOps(sessions[i].rcb, TRG_OPT_DATA_CHANGED | TRG_OPT_QUALITY_CHANGED | TRG_OPT_GI);
                ClientReportControlBlock_setDataSetReference(sessions[i].rcb, sessions[i].dataset);
                ClientReportControlBlock_setRptEna(sessions[i].rcb, true);
                ClientReportControlBlock_setGI(sessions[i].rcb, true);

                IedConnection_installReportHandler(
                    sessions[i].con,
                    sessions[i].dataset,
                    ClientReportControlBlock_getRptId(sessions[i].rcb),
                    reportCallbackFunction,
                    (void*) sessions[i].dataSetDirectory
                );

                IedConnection_setRCBValues(
                    sessions[i].con,
                    &error,
                    sessions[i].rcb,
                    RCB_ELEMENT_RPT_ENA | RCB_ELEMENT_GI,
                    true
                );

                if (error != IED_ERROR_OK) {
                    printf("Failed to configure RCB on %s (error: %d)\n", sessions[i].ip, error);
                    IedConnection_destroy(con);
                    sessions[i].connected = false;
                    continue;
                }

                sessions[i].connected = true;
                printf("Successfully reconnected to %s\n", sessions[i].ip);
                continue;
            }

            IedConnectionState state = IedConnection_getState(sessions[i].con);
            if (state != IED_STATE_CONNECTED) {
                printf("Session to %s disconnected.\n", sessions[i].ip);
                sessions[i].connected = false;

            } else {
                printf("Session to %s is connected.\n", sessions[i].ip);
                printf("Session %d: Dataset: %s, RCB: %s\n", i + 1,
                    sessions[i].dataset,
                    sessions[i].rcbName);
                MQTTClient_message pubmsg = MQTTClient_message_initializer;
                char tesSend[100] = "test";
                pubmsg.payload = tesSend;
                pubmsg.payloadlen = (int)strlen(tesSend);
                pubmsg.qos = MQTT_QOS;
                pubmsg.retained = 0;
                char topic[150];
                // printf("MachineCode: %s\n",);
                // snprintf(topic, sizeof(topic), "DMS/%s/IEC61850/Reports/%s", machine_code_, entryNameBuffer);



                MQTTClient_deliveryToken token;
                int rc = MQTTClient_publishMessage(mqttClient, "topic", &pubmsg, &token);
                if (rc == MQTTCLIENT_SUCCESS) {
                    MQTTClient_waitForCompletion(mqttClient, token, MQTT_TIMEOUT);
                    printf("MQTT report published successfully.\n"); //sads
                } else {
                    printf("MQTT publish failed: %d\n", rc);
                    initMqttClient();
                }
            }
        }

        Thread_sleep(5000);  // 1 second
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

    return 0;
}
