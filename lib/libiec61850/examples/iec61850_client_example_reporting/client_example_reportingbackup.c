/*
 * client_example_reporting.c
 *
 * Example client for IEC 61850 report handling and MQTT publishing.
 * Works with `server_example_basic_io` or `server_example_goose`.
 */

#include "iec61850_client.h"
#include "hal_thread.h"
#include <MQTTClient.h>
#include "src/cJSON.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <signal.h>
#include <inttypes.h>
#include <sys/time.h>  // For gettimeofday
#include <unistd.h>    // For usleep


#define MAX_REPORTS 5
#define MAX_HOSTS 10
#define MAX_SESSIONS 100

#define MAX_REPORTS_PER_DEVICE 5

#define MQTT_ADDRESS     "tcp://localhost:1883"
// #define MQTT_CLIENTID    "IEC61850_ReportClient"
#define MQTT_QOS         2
#define MQTT_TIMEOUT     10000L
#define MQTT_USERNAME "mgi"
#define MQTT_PASS "mel@2025"
MQTTClient mqttClient;

//// CONTROL VARIBLES ////
int disc_finished = 0;
int subscribed = 0;
int finished = 0;
char mqtt_topic_control_request[256];
char mqtt_topic_control_response[256];


bool TopicArrived = false;
const int mqttpayloadSize = 200;
char mqttpayload[200] = {'\0'};
char mqtttopic[30];

unsigned long lastTime = 0;
const unsigned long interval = 3000;  // 3 seconds

// MQTT

// SYSTEM
int master_id_device;
// SYSTEM


// CONTROL
#define MAX_ENABLED_CONTROLS 10
#define MAX_DATASETS 10

#define MAX_STRING_LEN 256
#define MAX_LIST_DATA 200  // Adjust based on expected max FCDA items


typedef struct
{
    char object[128];
    char ctlModel[64];
} EnabledControl;

typedef struct ReceiveControl
{
    char object[64];
    char valueNow[20];
    char lastValue[20];
    char typeData[32];
    char ctlCommand[32];
    bool synchrocheck;
    bool interlocking;
    bool testmode;
    int64_t timestamp;
} ReceiveControl;

typedef struct ResponseControl
{
    char object[64];
    char valueNow[20];
    char lastValue[20];
    char status[20];
    char ctlCommand[32];
    char iecErrorString[50];
    char errorString[40];
    char timestamp[40];
} ResponseControl;

typedef struct IecResponseControl
{
    char status[20];
    char iecErrorString[50];
    int timestamp;
} IecResponseControl;



EnabledControl enabledControls[MAX_ENABLED_CONTROLS];
int enabledCount = 0;
// CONTROL
////////CONTROL VARIBLES //////

static int running = 1;

char machine_code_[64] = "";
char id_device_[64] = ""; // Default value, can be overridden by config


typedef struct {
    int id;
    char fcda[128];
    char alias[128];
} FcdaEntry;

typedef struct {
    char dataset[128];
    char rcb[128];
    FcdaEntry fcdaList[MAX_LIST_DATA];
    int fcdaCount;
} ReportConfig;

typedef struct {
    char dataSetReference[MAX_STRING_LEN];
    FcdaEntry listData[MAX_LIST_DATA];
    int listDataCount;
} DataSetInfo;

// Global array to store all parsed datasets
DataSetInfo globalDataSets[MAX_DATASETS];
int globalDataSetCount = 0;


typedef struct {
    char ip[64];
    char port[64];
    ReportConfig reports[MAX_REPORTS];
    int reportCount;
    char machineCode[64];  // Optional field for machine code
    char id_device[64];    // Optional field for device ID
} HostConfig;

typedef struct {
    IedConnection con;
    ClientReportControlBlock rcbList[MAX_REPORTS_PER_DEVICE]; // You define the max
    LinkedList dataSetDirectoryList[MAX_REPORTS_PER_DEVICE];
    ClientDataSet clientDataSetList[MAX_REPORTS_PER_DEVICE];
    const char* datasetList[MAX_REPORTS_PER_DEVICE];
    const char* rcbNameList[MAX_REPORTS_PER_DEVICE];
    int reportCount;

    IedClientError error;
    const char* ip;
    int tcpPort;
    bool connected;
} ReportSession;

ReportSession sessions[MAX_SESSIONS];
int sessionCount = 0;
void message_arrived_callback(void *context, char *topicName, int topicLen, MQTTClient_message *message);
/* === Signal Handling === */
void sigint_handler(int signalId)
{
    running = 0;
}

unsigned long millis() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (unsigned long)(tv.tv_sec) * 1000 + (unsigned long)(tv.tv_usec) / 1000;
}


char mqtt_client_id[64];

void generate_mqtt_client_id() {
    const char charset[] = "0123456789ABCDEF";
    srand(time(NULL));
    char rand_suffix[9];
    for (int i = 0; i < 8; i++) {
        rand_suffix[i] = charset[rand() % 16];
    }
    rand_suffix[8] = '\0';
    snprintf(mqtt_client_id, sizeof(mqtt_client_id), "IEC61850_ReportClient_%s", rand_suffix);
}


void cleanupMqttClient()
{
    MQTTClient_disconnect(mqttClient, MQTT_TIMEOUT);
    MQTTClient_destroy(&mqttClient);
}

int initMqttClient()
{
    int rc;

    generate_mqtt_client_id();
    printf("Generated MQTT Client ID: %s\n", mqtt_client_id);
    rc = MQTTClient_create(&mqttClient, MQTT_ADDRESS, mqtt_client_id, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    if (rc != MQTTCLIENT_SUCCESS) {
        printf("MQTT client creation failed: %d\n", rc);
        return rc;
    }

    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    conn_opts.keepAliveInterval = 5;
    conn_opts.cleansession = 1;
    conn_opts.username = MQTT_USERNAME;
    conn_opts.password = MQTT_PASS;
    MQTTClient_setCallbacks(mqttClient, NULL, NULL, message_arrived_callback, NULL);
    rc = MQTTClient_connect(mqttClient, &conn_opts);
    if (rc != MQTTCLIENT_SUCCESS) {
        printf("MQTT connect failed: %d\n", rc);
        MQTTClient_destroy(&mqttClient);
        return rc;
    }

    //TODO: change the topic to dynamic based on id_device and machine code
    // sprintf(mqtt_topic_control_request, "+/%d/control/request", 1);
    // sprintf(mqtt_topic_control_response, "DMS/%d/control/response", 1);
    rc = MQTTClient_subscribe(mqttClient, mqtt_topic_control_request, 1);
    if (rc != MQTTCLIENT_SUCCESS)
    {
        fprintf(stderr, "MQTT: Failed to subscribe to topic %s, return code %d\n", mqtt_topic_control_request, rc);
    }
    else
    {
        printf("MQTT: Subscribed to topic %s\n", mqtt_topic_control_request);
        TopicArrived = false;
    }
    return MQTTCLIENT_SUCCESS;
}

const char* getAliasFromGlobalDataset(const char* fcdaName) {
    for (int i = 0; i < globalDataSetCount; i++) {
        DataSetInfo* dataset = &globalDataSets[i];

        for (int j = 0; j < dataset->listDataCount; j++) {
            if (strcmp(dataset->listData[j].fcda, fcdaName) == 0) {
                return dataset->listData[j].alias;
            }
        }
    }

    return NULL; // Alias not found
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
    char entryNameBuffer[128] = "";

    if(dataSetDirectory == NULL || LinkedList_size(dataSetDirectory) == 0) {
        printf("No data set directory available.\n");
        return;
    }

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

        // printf("Processing entry: %s, reason: %d, value: %s\n", entryNameBuffer, reason, valBuffer);

        const char* alias = getAliasFromGlobalDataset(entryNameBuffer);
        // if (alias) {
        //     printf("Alias for %s is %s\n", entryNameBuffer, alias);
        // } else {
        //     printf("Alias for %s not found.\n", entryNameBuffer);
        // }

        // Find the opening brace
        char* start = strchr(valBuffer, '{');
        if (start) {
            // Move past the opening bracef
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

        // printf("First value: %s\n", firstValue);

        char line[600];
        snprintf(line, sizeof(line),
        "{value: %s, alias: %s, \"timestamp\": \"%s\" }", firstValue, alias,timestampBuf);

        // printf("entry: %s, reason: %d, value: %s\n", entryName, reason, valBuffer);
        // printf("Formatted timestamp: %s\n", timestampBuf);

        MQTTClient_message pubmsg = MQTTClient_message_initializer;
        pubmsg.payload = line;
        pubmsg.payloadlen = (int)strlen(line);
        pubmsg.qos = MQTT_QOS;
        pubmsg.retained = 0;
        char topic[512];
        // printf("MachineCode: %s\n",);
        // printf("id_device_: %s\n", id_device_);
        snprintf(topic, sizeof(topic), "DMS/%s/IEC61850/Reports/%s/%s", machine_code_,id_device_,entryNameBuffer);



        MQTTClient_deliveryToken token;
        int rc = MQTTClient_publishMessage(mqttClient, topic, &pubmsg, &token);
        if (rc == MQTTCLIENT_SUCCESS) {
            MQTTClient_waitForCompletion(mqttClient, token, MQTT_TIMEOUT);
            // printf("MQTT report published successfully.\n"); //sads
        } else {
            printf("MQTT publish failed: %d\n", rc);
        }
    }
}

/* === Config Loader === */
int loadHostConfigs(const char* filename, HostConfig* host)
{
    char configFile[256];
    snprintf(configFile, sizeof(configFile), "%s_parsed.json", filename);

    FILE* fp = fopen(configFile, "r");
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
    cJSON* id_device = cJSON_GetObjectItem(root, "idDevice");

    if (!ip || !port) {
        cJSON_Delete(root);
        return 0;
    }

    strncpy(host->ip, ip->valuestring, sizeof(host->ip));
    strncpy(host->port, port->valuestring, sizeof(host->port));
    strncpy(host->machineCode, machine_code ? machine_code->valuestring : "", sizeof(host->machineCode));
    strncpy(host->id_device, id_device ? id_device->valuestring : "", sizeof(host->id_device));

    cJSON* reportList = cJSON_GetObjectItem(root, "reports");
    if (!reportList || !cJSON_IsArray(reportList)) {
        cJSON_Delete(root);
        return 0;
    }

    // Load CConfig3.json
    char datasetFile[256];
    snprintf(datasetFile, sizeof(datasetFile), "%s_datasets.json", filename);
    FILE* cfg = fopen(datasetFile, "r");
    if (!cfg) {
        perror("CConfig3.json open failed");
        cJSON_Delete(root);
        return 0;
    }

    fseek(cfg, 0, SEEK_END);
    long cfg_len = ftell(cfg);
    rewind(cfg);

    char* cfg_data = malloc(cfg_len + 1);
    fread(cfg_data, 1, cfg_len, cfg);
    cfg_data[cfg_len] = '\0';
    fclose(cfg);

    cJSON* configRoot = cJSON_Parse(cfg_data);
    free(cfg_data);
    if (!configRoot || !cJSON_IsArray(configRoot)) {
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

            // Match with CConfig3.json
            cJSON* configEntry = NULL;
            cJSON_ArrayForEach(configEntry, configRoot) {
                cJSON* ref = cJSON_GetObjectItem(configEntry, "dataSetReference");
                if (ref && strcmp(ref->valuestring, dsRef->valuestring) == 0) {
                    cJSON* listData = cJSON_GetObjectItem(configEntry, "listData");
                    if (listData && cJSON_IsArray(listData)) {
                        strncpy(globalDataSets[globalDataSetCount].dataSetReference, ref->valuestring, sizeof(globalDataSets[globalDataSetCount].dataSetReference));

                        int globalFcdaIdx = 0;
                        cJSON* fcdaEntry = NULL;
                        cJSON_ArrayForEach(fcdaEntry, listData) {
                            if (globalFcdaIdx >= MAX_LIST_DATA) break;

                            cJSON* id = cJSON_GetObjectItem(fcdaEntry, "id");
                            cJSON* fcda = cJSON_GetObjectItem(fcdaEntry, "fcda");
                            cJSON* alias = cJSON_GetObjectItem(fcdaEntry, "alias");

                            if (id && fcda && alias) {
                                globalDataSets[globalDataSetCount].listData[globalFcdaIdx].id = id->valueint;
                                strncpy(globalDataSets[globalDataSetCount].listData[globalFcdaIdx].fcda, fcda->valuestring, sizeof(globalDataSets[globalDataSetCount].listData[globalFcdaIdx].fcda));
                                strncpy(globalDataSets[globalDataSetCount].listData[globalFcdaIdx].alias, alias->valuestring, sizeof(globalDataSets[globalDataSetCount].listData[globalFcdaIdx].alias));
                                globalFcdaIdx++;
                            }
                        }
                        globalDataSets[globalDataSetCount].listDataCount = globalFcdaIdx;
                        globalDataSetCount++;
                    }
                    break; // Match found
                }
            }

            printf("Loaded report %d: DataSet=%s, RCB=%s, FCDA count: %d\n",
                   reportIndex, host->reports[reportIndex].dataset,
                   host->reports[reportIndex].rcb,
                   host->reports[reportIndex].fcdaCount);

            reportIndex++;
        }
    }

    host->reportCount = reportIndex;

    // Process enabled controls
    cJSON* controlArray = cJSON_GetObjectItem(root, "control");
    if (cJSON_IsArray(controlArray)) {
        int size = cJSON_GetArraySize(controlArray);
        for (int i = 0; i < size; i++) {
            cJSON* item = cJSON_GetArrayItem(controlArray, i);
            cJSON* enabled = cJSON_GetObjectItem(item, "enabled");
            if (cJSON_IsTrue(enabled)) {
                cJSON* object = cJSON_GetObjectItem(item, "object");
                cJSON* ctlModel = cJSON_GetObjectItem(item, "ctlModel");
                if (object && ctlModel) {
                    strncpy(enabledControls[enabledCount].object, object->valuestring, sizeof(enabledControls[enabledCount].object) - 1);
                    strncpy(enabledControls[enabledCount].ctlModel, ctlModel->valuestring, sizeof(enabledControls[enabledCount].ctlModel) - 1);
                    enabledCount++;
                }
            }
        }
    }

    printf("Found %d enabled control items:\n", enabledCount);
    for (int i = 0; i < enabledCount; i++) {
        printf(" - Object: %s | ctlModel: %s\n", enabledControls[i].object, enabledControls[i].ctlModel);
    }

    cJSON_Delete(configRoot);
    cJSON_Delete(root);
    return 1;
}



////////////////////// CONTROL ??////////////////////////



// FUNCTION
int IEC61850_control_direct_security(IedConnection con, char *control_obj, bool value);
int IEC61850_control_direct_security_ex(IedConnection con, char *control_obj, ReceiveControl rc, ResponseControl *resp);
int IEC61850_control_direct_security_exp(IedConnection con, char *control_obj, bool value);
int IEC61850_control_direct_security_exp_ex(IedConnection con, char *control_obj, ReceiveControl rc, ResponseControl *resp);
int IEC61850_control_direct_normal(IedConnection con, char *control_obj, bool value);
int IEC61850_control_direct_normal_ex(IedConnection con, char *control_obj, ReceiveControl rc, ResponseControl *resp);
int IEC61850_control_sbo_normal(IedConnection con, char *control_obj, bool value);
int IEC61850_control_sbo_normal_ex(IedConnection con, char *control_obj, ReceiveControl rc, ResponseControl *resp);
int IEC61850_control_sbo_security(IedConnection con, char *control_obj, bool value);
int IEC61850_control_sbo_security_ex(IedConnection con, char *control_obj, ReceiveControl rc, ResponseControl *resp);
int IEC61850_control_cancel(IedConnection con, char *control_obj);
int IEC61850_control_cancel_ex(IedConnection con, char *control_obj, ResponseControl *iecRc);
bool str_ends_with(const char *str, const char *suffix);
const char *ControlAddCause_toString(ControlAddCause cause);
const char *ControlLastApplError_toString(ControlLastApplError error);
const char *IEC61850_GetcommandLastApplError_ApplError(ControlObjectClient ctlCommand);
const char *IEC61850_GetcommandLastApplError_AddCause(ControlObjectClient ctlCommand);
const char *IEC61850_GetcommandLastErrorString(ControlObjectClient ctlCommand);
// FUNCTION

const char *current_time_str()
{
    static char buffer[32]; // Static so it's valid after return
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    // printf("tm_info: %p\n", tm_info);

    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info);
    return buffer;
}

const char *current_time_str_tz()
{
    static char buffer[64];
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now); // localtime includes timezone info

    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S %Z %z", tm_info);
    return buffer;
}

const char *current_time_iso8601_ms_local()
{
    static char buffer[64];
    struct timespec ts;

    // Use CLOCK_REALTIME to get current local time (system time)
    clock_gettime(CLOCK_REALTIME, &ts);

    struct tm tm_info;
    localtime_r(&ts.tv_sec, &tm_info); // Convert to local time (not UTC)

    int millis = ts.tv_nsec / 1000000; // Get milliseconds (divide nanoseconds by 1 million)

    snprintf(buffer, sizeof(buffer), "%04d-%02d-%02dT%02d:%02d:%02d.%03d",
             tm_info.tm_year + 1900, tm_info.tm_mon + 1, tm_info.tm_mday,
             tm_info.tm_hour, tm_info.tm_min, tm_info.tm_sec, millis);

    return buffer;
}

const char *current_time_iso8601_ms_utc()
{
    static char buffer[64];
    struct timespec ts;

    clock_gettime(CLOCK_REALTIME, &ts); // Get current UTC time (nanosecond precision)

    struct tm tm_info;
    gmtime_r(&ts.tv_sec, &tm_info); // Convert to UTC tm structure

    int millis = ts.tv_nsec / 1000000; // Get milliseconds (divide nanoseconds by 1 million)

    snprintf(buffer, sizeof(buffer), "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ",
             tm_info.tm_year + 1900, tm_info.tm_mon + 1, tm_info.tm_mday,
             tm_info.tm_hour, tm_info.tm_min, tm_info.tm_sec, millis);

    return buffer;
}

#define MAX_QUEUE 100
#define MAX_PAYLOAD 4096

typedef struct
{
    char topic[256];
    char payload[MAX_PAYLOAD];
} Message;

Message mqttSubData;

// Check if a string ends with another string
bool str_ends_with(const char *str, const char *suffix)
{
    if (!str || !suffix)
        return false;

    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);

    if (suffix_len > str_len)
        return false;

    return strncmp(str + str_len - suffix_len, suffix, suffix_len) == 0;
}

int is_valid_json(const char *payload)
{
    if (payload == NULL)
        return 0;

    cJSON *json = cJSON_Parse(payload);
    if (json == NULL)
    {
        return 0; // Not valid JSON
    }

    cJSON_Delete(json);
    return 1; // Valid JSON
}

// subscribe callback print
void message_arrived_callback(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    printf("Message arrived on topic :%s\n", topicName);
    // if (/*strstr(topicName, "control") == NULL &&*/ strstr(topicName, "request") == NULL)
    // {
    //     printf("Wrong Topic: %s\n", topicName);
    //     MQTTClient_freeMessage(&message);
    //     MQTTClient_free(topicName);
    //     return 1;
    // }

    if (is_valid_json((char *)message->payload))
    {
        printf(" Requested Control %s: %.*s\n", topicName, message->payloadlen, (char *)message->payload);
        // char *payload = (char *)message->payload;
        strncpy(mqttSubData.topic, topicName, sizeof(mqttSubData.topic) - 1);
        strncpy(mqttSubData.payload, (char *)message->payload, sizeof(mqttSubData.payload) - 1);
        // enqueueMessage(topicName, payload);
        TopicArrived = true;
    }
    else
        printf("Invalid JSON\n");

    // MQTTClient_freeMessage(&message);
    // MQTTClient_free(topicName);
}

int processParseJsonReceive(char *payload, ReceiveControl *out)
{
    cJSON *root = cJSON_Parse(payload);
    if (!root)
    {
        fprintf(stderr, "Error before: [%s]\n", cJSON_GetErrorPtr());
        free(root);
        return 1;
    }
    else
    {
        // value
        cJSON *value = cJSON_GetObjectItem(root, "value");
        if (value)
        {
            if (cJSON_IsNumber(value))
            {
                printf("value (number): %d\n", value->valueint);
            }
            else if (cJSON_IsBool(value))
            {
                printf("value (bool): %s\n", cJSON_IsTrue(value) ? "true" : "false");
            }
            else if (cJSON_IsString(value))
            {
                printf("value (string): %s\n", value->valuestring);
            }
            else
            {
                printf("Warning: 'value' has unsupported type\n");
            }
        }
        else
        {
            printf("Warning: 'value' missing\n");
        }

        // ctlCommand
        cJSON *ctlCommand = cJSON_GetObjectItem(root, "ctlCommand");
        if (cJSON_IsString(ctlCommand))
        {
            printf("ctlCommand: %s\n", ctlCommand->valuestring);
        }
        else
        {
            printf("Error: Missing or invalid 'ctlCommand'\n");
            cJSON_Delete(root);
            return -2;
        }

        // lastValue
        cJSON *lastValue = cJSON_GetObjectItem(root, "lastValue");
        if (lastValue)
        {
            if (cJSON_IsNumber(lastValue))
            {
                printf("lastValue (number): %f\n", lastValue->valuedouble);
            }
            else if (cJSON_IsBool(lastValue))
            {
                printf("lastValue (bool): %s\n", cJSON_IsTrue(lastValue) ? "true" : "false");
            }
            else if (cJSON_IsString(lastValue))
            {
                printf("lastValue (string): %s\n", lastValue->valuestring);
            }
            else
            {
                printf("Warning: 'lastValue' has unsupported type\n");
            }
        }
        else
        {
            printf("Warning: 'lastValue' missing\n");
        }

        // interlocking
        cJSON *interlocking = cJSON_GetObjectItem(root, "interlocking");
        if (cJSON_IsBool(interlocking))
        {
            printf("interlocking: %s\n", cJSON_IsTrue(interlocking) ? "true" : "false");
        }
        else
        {
            printf("Error: Missing or invalid 'interlocking'\n");
            cJSON_Delete(root);
            return -3;
        }

        // synchrocheck
        cJSON *synchrocheck = cJSON_GetObjectItem(root, "synchrocheck");
        if (cJSON_IsBool(synchrocheck))
        {
            printf("synchrocheck: %s\n", cJSON_IsTrue(synchrocheck) ? "true" : "false");
        }
        else
        {
            printf("Error: Missing or invalid 'synchrocheck'\n");
            cJSON_Delete(root);
            return -4;
        }

        // testmode
        cJSON *testmode = cJSON_GetObjectItem(root, "testmode");
        if (cJSON_IsBool(testmode))
        {
            printf("testmode: %s\n", cJSON_IsTrue(testmode) ? "true" : "false");
        }
        else
        {
            printf("Error: Missing or invalid 'testmode'\n");
            cJSON_Delete(root);
            return -5;
        }

        // timestamp
        cJSON *timestamp = cJSON_GetObjectItem(root, "timestamp");
        if (cJSON_IsNumber(timestamp))
        {
            int64_t ts = (int64_t)timestamp->valuedouble;
            printf("timestamp: %" PRId64 "\n", ts);
        }
        else
        {
            printf("Error: Missing or invalid 'timestamp'\n");
            cJSON_Delete(root);
            return -6;
        }

        cJSON_Delete(root);
    }
}

int parseJsonToReceiveControl(const char *jsonStr, ReceiveControl *rc)
{
    if (jsonStr == NULL || rc == NULL)
    {
        return -1; // invalid input
    }

    cJSON *root = cJSON_Parse(jsonStr);
    if (root == NULL)
    {
        return -2; // invalid JSON
    }

// Helper function to copy string safely
#define COPY_STRING_FIELD(field, jsonObj, fieldName, maxLen)                 \
    do                                                                       \
    {                                                                        \
        cJSON *item = cJSON_GetObjectItem(jsonObj, fieldName);               \
        if (cJSON_IsString(item) && (item->valuestring != NULL))             \
        {                                                                    \
            strncpy(rc->field, item->valuestring, maxLen - 1);               \
            rc->field[maxLen - 1] = '\0';                                    \
        }                                                                    \
        else                                                                 \
        {                                                                    \
            rc->field[0] = '\0'; /* empty string if missing or wrong type */ \
        }                                                                    \
    } while (0)

    // Parse typeData
    COPY_STRING_FIELD(object, root, "object", sizeof(rc->object));

    // Parse typeData
    COPY_STRING_FIELD(typeData, root, "typeData", sizeof(rc->typeData));

    // Parse ctlCommand
    COPY_STRING_FIELD(ctlCommand, root, "ctlCommand", sizeof(rc->ctlCommand));

    // Parse value and lastValue - convert any JSON type to string
    cJSON *valueItem = cJSON_GetObjectItem(root, "valueNow");
    if (valueItem)
    {
        if (cJSON_IsBool(valueItem))
        {
            snprintf(rc->valueNow, sizeof(rc->valueNow), "%s", cJSON_IsTrue(valueItem) ? "true" : "false");
        }
        else if (cJSON_IsNumber(valueItem))
        {
            snprintf(rc->valueNow, sizeof(rc->valueNow), "%g", valueItem->valuedouble);
        }
        else if (cJSON_IsString(valueItem))
        {
            strncpy(rc->valueNow, valueItem->valuestring, sizeof(rc->valueNow) - 1);
            rc->valueNow[sizeof(rc->valueNow) - 1] = '\0';
        }
        else
        {
            rc->valueNow[0] = '\0';
        }
    }
    else
    {
        rc->valueNow[0] = '\0';
    }

    cJSON *lastValueItem = cJSON_GetObjectItem(root, "lastValue");
    if (lastValueItem)
    {
        if (cJSON_IsBool(lastValueItem))
        {
            snprintf(rc->lastValue, sizeof(rc->lastValue), "%s", cJSON_IsTrue(lastValueItem) ? "true" : "false");
        }
        else if (cJSON_IsNumber(lastValueItem))
        {
            snprintf(rc->lastValue, sizeof(rc->lastValue), "%g", lastValueItem->valuedouble);
        }
        else if (cJSON_IsString(lastValueItem))
        {
            strncpy(rc->lastValue, lastValueItem->valuestring, sizeof(rc->lastValue) - 1);
            rc->lastValue[sizeof(rc->lastValue) - 1] = '\0';
        }
        else
        {
            rc->lastValue[0] = '\0';
        }
    }
    else
    {
        rc->lastValue[0] = '\0';
    }

    // Parse booleans, default false if missing or wrong type
    cJSON *interlockingItem = cJSON_GetObjectItem(root, "interlocking");
    rc->interlocking = (interlockingItem && cJSON_IsBool(interlockingItem) && cJSON_IsTrue(interlockingItem)) ? true : false;

    cJSON *synchrocheckItem = cJSON_GetObjectItem(root, "synchrocheck");
    rc->synchrocheck = (synchrocheckItem && cJSON_IsBool(synchrocheckItem) && cJSON_IsTrue(synchrocheckItem)) ? true : false;

    cJSON *testmodeItem = cJSON_GetObjectItem(root, "testmode");
    rc->testmode = (testmodeItem && cJSON_IsBool(testmodeItem) && cJSON_IsTrue(testmodeItem)) ? true : false;

    // Parse timestamp (int64)
    cJSON *timestampItem = cJSON_GetObjectItem(root, "timestamp");
    if (timestampItem && cJSON_IsNumber(timestampItem))
    {
        rc->timestamp = (int64_t)timestampItem->valuedouble;
    }
    else
    {
        rc->timestamp = 0;
    }

    cJSON_Delete(root);
    return 0; // success
}

void processIEC61850Control(IedConnection iecConn, const char *ctlModel, ReceiveControl rc, ResponseControl *iecRc)
{

    if (strcmp(rc.ctlCommand, "cancel") == 0)
    {
        // log_info("IEC61850: Control :%s", "cancel command");

        ResponseControl RespCancel;
        IEC61850_control_cancel_ex(iecConn, rc.object, &RespCancel);
        memcpy(iecRc, &RespCancel, sizeof(ResponseControl));

    }
    else
    {
        IedConnectionState state = IedConnection_getState(iecConn);
         if (state != IED_STATE_CONNECTED) {
                printf("Disconnected from server. Will attempt to reconnect.\n");
                // sessions[i].connected = false;
            }
        printf("ReceiveControl: %s\n", rc.object);
        if (strcmp(ctlModel, "direct-with-normal-security") == 0)
        {
            ResponseControl RespDirectNormal;
            // log_info("IEC61850: Control :%s", "direct normal command");
            IEC61850_control_direct_normal_ex(iecConn, rc.object, rc, &RespDirectNormal);
            memcpy(iecRc, &RespDirectNormal, sizeof(ResponseControl));
        }
        if (strcmp(ctlModel, "sbo-with-normal-security") == 0)
        {
            ResponseControl RespSboNormal;
            // log_info("IEC61850: Control :%s", "sbo normal command");
            IEC61850_control_sbo_normal_ex(iecConn, rc.object, rc, &RespSboNormal);
            memcpy(iecRc, &RespSboNormal, sizeof(ResponseControl));
        }
        if (strcmp(ctlModel, "direct-with-enhanced-security") == 0)
        {
            ResponseControl RespDirectSecurity;
            // log_info("IEC61850: Control :%s", "direct security command");
            IEC61850_control_direct_security_ex(iecConn, rc.object, rc, &RespDirectSecurity);
            memcpy(iecRc, &RespDirectSecurity, sizeof(ResponseControl));
        }
        if (strcmp(ctlModel, "sbo-with-enhanced-security") == 0)
        {
            ResponseControl RespSboSecurity;
            // log_info("IEC61850: Control :%s", "sbo security command");
            IEC61850_control_sbo_security_ex(iecConn, rc.object, rc, &RespSboSecurity);
            memcpy(iecRc, &RespSboSecurity, sizeof(ResponseControl));
        }
    }
}

int MQTT_publish(MQTTClient mqttClient, const char *topic, const char *msg)
{
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    pubmsg.payload = (void *)msg;
    pubmsg.payloadlen = (int)strlen(msg);
    pubmsg.qos = MQTT_QOS;
    pubmsg.retained = 0;

    // MQTTClient_waitForCompletion(mqttClient, token, MQTT_TIMEOUT);
    MQTTClient_deliveryToken token;
        int rc = MQTTClient_publishMessage(mqttClient, topic, &pubmsg, &token);
        if (rc == MQTTCLIENT_SUCCESS) {
            MQTTClient_waitForCompletion(mqttClient, token, MQTT_TIMEOUT);
            printf("MQTT report published successfully Control.\n"); //sads
        } else {
            printf("MQTT publish failed: %d\n", rc);
        }

    return 0;
}

int processResponseControlToJson(ResponseControl *resp, char *outputJson, size_t maxLen)
{
    if (resp == NULL || outputJson == NULL)
        return -1;

    cJSON *root = cJSON_CreateObject();
    if (!root)
        return -1;

    cJSON_AddStringToObject(root, "valueNow", resp->valueNow);
    cJSON_AddStringToObject(root, "lastValue", resp->lastValue);
    cJSON_AddStringToObject(root, "status", resp->status);
    cJSON_AddStringToObject(root, "object", resp->object);
    cJSON_AddStringToObject(root, "ctlCommand", resp->ctlCommand);
    cJSON_AddStringToObject(root, "iecErrorString", resp->iecErrorString);
    cJSON_AddStringToObject(root, "errorString", resp->errorString);
    cJSON_AddStringToObject(root, "timestamp", resp->timestamp);

    char *jsonStr = cJSON_PrintUnformatted(root);
    // printf("%s\n", jsonStr);
    if (!jsonStr)
    {
        cJSON_Delete(root);
        return -1;
    }

    // Copy result to output buffer safely
    strncpy(outputJson, jsonStr, maxLen - 1);
    outputJson[maxLen - 1] = '\0'; // Null-terminate

    // Clean up
    free(jsonStr);
    cJSON_Delete(root);
    return 0;
}

void processMessages(IedConnection iecConn, MQTTClient mqttClient)
{
    // Message msg;
    // while (dequeueMessage(&msg))
    // {
    // printf("Processing message from topic\n");
    char respJsonOutput[512];
    if (TopicArrived)
    {
        printf("TopicArrived: TRUE\n");
        // log_info("MQTT: Topic received ->  %s", msg.topic);
        // log_info("MQTT: Message received ->%s", msg.payload);
        // printf("DEQUEUE MESSAGE\n");

        ReceiveControl revCtrlObj;
        int ret = parseJsonToReceiveControl(mqttSubData.payload, &revCtrlObj);
        if (ret == 0)
        {
            // printf("Parsed ReceiveControl:\n");
            // printf(" object: %s\n", revCtrlObj.object);
            // printf(" valueNow: %s\n", revCtrlObj.valueNow);
            // printf(" lastValue: %s\n", revCtrlObj.lastValue);
            // printf(" typeData: %s\n", revCtrlObj.typeData);
            // printf(" ctlCommand: %s\n", revCtrlObj.ctlCommand);
            // printf(" interlocking: %s\n", revCtrlObj.interlocking ? "true" : "false");
            // printf(" synchrocheck: %s\n", revCtrlObj.synchrocheck ? "true" : "false");
            // printf(" testmode: %s\n", revCtrlObj.testmode ? "true" : "false");
            // printf(" timestamp: %" PRId64 "\n", revCtrlObj.timestamp);

            ResponseControl RespCtl;
            int checkObject = 1;
            for (int i = 0; i < enabledCount; i++)
            {
                // printf("Checking enabled control: %s\n", enabledControls[i].object);
                if (strcmp(revCtrlObj.object, enabledControls[i].object) == 0)
                {
                    processIEC61850Control(iecConn, enabledControls[i].ctlModel, revCtrlObj, &RespCtl);
                    checkObject = 0;
                    break;
                }
                else
                {
                    checkObject = 1;
                }
            }
            printf("checkObject: %d\n", checkObject);
            if (checkObject != 0)
            {
                strcpy(RespCtl.valueNow, revCtrlObj.valueNow);
                strcpy(RespCtl.ctlCommand, revCtrlObj.ctlCommand);
                strcpy(RespCtl.object, revCtrlObj.object);
                strcpy(RespCtl.lastValue, revCtrlObj.lastValue);
                strcpy(RespCtl.status, "error");
                strcpy(RespCtl.errorString, "none");
                strcpy(RespCtl.iecErrorString, "none");
                strcpy(RespCtl.errorString, "item not found in device");
                strcpy(RespCtl.timestamp, current_time_str());
            }
            ret = processResponseControlToJson(&RespCtl, respJsonOutput, sizeof(respJsonOutput));

            if (ret != 0)
            {
                printf("Failed to generate JSON.\n");
            }
            // MQTT_publish(mqttClient, mqtt_topic_control_response, respJsonOutput);
            // printf("Publishing response to MQTT topic: %s\n", mqtt_topic_control_response);
            MQTTClient_message pubmsg = MQTTClient_message_initializer;
            char tesSend[100] = "test";
            pubmsg.payload = respJsonOutput;
            pubmsg.payloadlen = (int)strlen(respJsonOutput);
            pubmsg.qos = MQTT_QOS;
            pubmsg.retained = 0;
            char topic[150];
            // printf("MachineCode: %s\n",);
            // snprintf(topic, sizeof(topic), "DMS/%s/IEC61850/Reports/%s", machine_code_, entryNameBuffer);



            MQTTClient_deliveryToken token;
            int rc = MQTTClient_publishMessage(mqttClient, mqtt_topic_control_response, &pubmsg, &token);
            if (rc == MQTTCLIENT_SUCCESS) {
                MQTTClient_waitForCompletion(mqttClient, token, MQTT_TIMEOUT);
                // printf("MQTT report published successfully.\n"); //sads
                printf("MQTT publish successful Control: %s\n", respJsonOutput);
            } else {
                printf("MQTT publish failed Control: %d\n", rc);
                // initMqttClient();
            }
        }
        else
        {
            printf("Failed to parse JSON: %d\n", ret);
        }

        TopicArrived = false;
        printf("TopicArrived: FALSE\n");
    }
}

// MQTT

//// ERROR HANDLING IEC61850///////////
const char *ControlAddCause_toString(ControlAddCause cause)
{
    switch (cause)
    {
    case ADD_CAUSE_UNKNOWN:
        return "Unknown";
    case ADD_CAUSE_NOT_SUPPORTED:
        return "Not Supported";
    case ADD_CAUSE_BLOCKED_BY_SWITCHING_HIERARCHY:
        return "Blocked by Switching Hierarchy";
    case ADD_CAUSE_SELECT_FAILED:
        return "Select Failed";
    case ADD_CAUSE_INVALID_POSITION:
        return "Invalid Position";
    case ADD_CAUSE_POSITION_REACHED:
        return "Position Reached";
    case ADD_CAUSE_PARAMETER_CHANGE_IN_EXECUTION:
        return "Parameter Change in Execution";
    case ADD_CAUSE_STEP_LIMIT:
        return "Step Limit Reached";
    case ADD_CAUSE_BLOCKED_BY_MODE:
        return "Blocked by Mode";
    case ADD_CAUSE_BLOCKED_BY_PROCESS:
        return "Blocked by Process";
    case ADD_CAUSE_BLOCKED_BY_INTERLOCKING:
        return "Blocked by Interlocking";
    case ADD_CAUSE_BLOCKED_BY_SYNCHROCHECK:
        return "Blocked by Synchrocheck";
    case ADD_CAUSE_COMMAND_ALREADY_IN_EXECUTION:
        return "Command Already in Execution";
    case ADD_CAUSE_BLOCKED_BY_HEALTH:
        return "Blocked by Health";
    case ADD_CAUSE_1_OF_N_CONTROL:
        return "1 of N Control";
    case ADD_CAUSE_ABORTION_BY_CANCEL:
        return "Abortion by Cancel";
    case ADD_CAUSE_TIME_LIMIT_OVER:
        return "Time Limit Over";
    case ADD_CAUSE_ABORTION_BY_TRIP:
        return "Abortion by Trip";
    case ADD_CAUSE_OBJECT_NOT_SELECTED:
        return "Object Not Selected";
    case ADD_CAUSE_OBJECT_ALREADY_SELECTED:
        return "Object Already Selected";
    case ADD_CAUSE_NO_ACCESS_AUTHORITY:
        return "No Access Authority";
    case ADD_CAUSE_ENDED_WITH_OVERSHOOT:
        return "Ended with Overshoot";
    case ADD_CAUSE_ABORTION_DUE_TO_DEVIATION:
        return "Abortion Due to Deviation";
    case ADD_CAUSE_ABORTION_BY_COMMUNICATION_LOSS:
        return "Abortion by Communication Loss";
    case ADD_CAUSE_ABORTION_BY_COMMAND:
        return "Abortion by Command";
    case ADD_CAUSE_NONE:
        return "None";
    case ADD_CAUSE_INCONSISTENT_PARAMETERS:
        return "Inconsistent Parameters";
    case ADD_CAUSE_LOCKED_BY_OTHER_CLIENT:
        return "Locked by Other Client";
    default:
        return "Invalid Add Cause";
    }
}

const char *ControlLastApplError_toString(ControlLastApplError error)
{
    switch (error)
    {
    case CONTROL_ERROR_NO_ERROR:
        return "No Error";
    case CONTROL_ERROR_UNKNOWN:
        return "Unknown Error";
    case CONTROL_ERROR_TIMEOUT_TEST:
        return "Timeout Test";
    case CONTROL_ERROR_OPERATOR_TEST:
        return "Operator Test";
    default:
        return "Invalid Application Error";
    }
}

const char *IEC61850_GetcommandLastApplError_ApplError(ControlObjectClient ctlCommand)
{
    LastApplError lastApplError = ControlObjectClient_getLastApplError(ctlCommand);
    return ControlLastApplError_toString(lastApplError.error);
}

const char *IEC61850_GetcommandLastApplError_AddCause(ControlObjectClient ctlCommand)
{
    LastApplError lastApplError = ControlObjectClient_getLastApplError(ctlCommand);
    return ControlAddCause_toString(lastApplError.addCause);
}

const char *IEC61850_GetcommandLastErrorString(ControlObjectClient ctlCommand)
{
    IedClientError lastError = ControlObjectClient_getLastError(ctlCommand);
    return IedClientError_toString(lastError);
}

void IEC61850_printError(ControlObjectClient ctlCommand)
{
    printf("%s\n", IEC61850_GetcommandLastErrorString(ctlCommand));
    printf("%s\n", IEC61850_GetcommandLastApplError_ApplError(ctlCommand));
    printf("%s\n", IEC61850_GetcommandLastApplError_AddCause(ctlCommand));
}
//// ERROR HANDLING ///////////

static void commandTerminationHandler(void *parameter, ControlObjectClient connection)
{

    LastApplError lastApplError = ControlObjectClient_getLastApplError(connection);

    /* if lastApplError.error != 0 this indicates a CommandTermination- */
    if (lastApplError.error != 0)
    {
        printf("Received CommandTermination-.\n");
        printf(" LastApplError: %i\n", lastApplError.error);
        printf("      addCause: %i\n", lastApplError.addCause);
    }
    else
        printf("Received CommandTermination+.\n");
}

MmsValue *createDynamicMMS(char *_type, char *value)
{
    // log_info("VALUE: %s", value);
    if (strcmp(_type, "boolean") == 0)
    {
        if (strcmp(value, "false") == 0)
            return MmsValue_newBoolean(false);
        else if (strcmp(value, "true") == 0)
            return MmsValue_newBoolean(true);
    }
    if (strcmp(_type, "bit-string") == 0)
    {
        int intBitstring = atoi(value);
        MmsValue *valBitString = MmsValue_newBitString(8);
        // MmsValue_setBitStringFromInteger(valBitString, intBitstring);
        return valBitString;
    }
    if (strcmp(_type, "integer") == 0)
        return MmsValue_newInteger(atoi(value));
}

int IEC61850_control_direct_normal_ex(IedConnection con, char *control_obj, ReceiveControl rc, ResponseControl *resp)
{
    MmsValue *ctlVal = NULL;
    IedClientError error;

    /************************
     * Direct control
     ***********************/
    ControlObjectClient control = ControlObjectClient_create(control_obj, con);
    strcpy(resp->ctlCommand, "direct");
    strcpy(resp->object, control_obj);
    strcpy(resp->lastValue, "none");
    strcpy(resp->iecErrorString, "none");
    strcpy(resp->errorString, "none");

    if (control)
    {
        // ctlVal = MmsValue_newBoolean(true);
        ctlVal = createDynamicMMS(rc.typeData, rc.valueNow);
        strcpy(resp->valueNow, rc.valueNow);

        ControlObjectClient_setOrigin(control, NULL, 3);

        if (ControlObjectClient_operate(control, ctlVal, 0 /* operate now */))
        {
            printf("%s operated successfully\n", control_obj);
            strcpy(resp->status, "success");
        }
        else
        {
            printf("failed to operate %s\n", control_obj);
            strcpy(resp->status, "failed");
            strcpy(resp->errorString, "value not changed");
        }
        MmsValue_delete(ctlVal);
        ControlObjectClient_destroy(control);
    }
    else
    {
        printf("Control object %s not found in server\n", control_obj);
        strcpy(resp->status, "failed");
        strcpy(resp->errorString, "item not found");
    }
    strcpy(resp->timestamp, current_time_str());
}

int IEC61850_control_sbo_normal_ex(IedConnection con, char *control_obj, ReceiveControl rc, ResponseControl *resp)
{
    MmsValue *ctlVal = NULL;
    MmsValue *stVal = NULL;
    /************************
     * Select before operate
     ***********************/
    const char *select_normal_ctl_obj = control_obj;
    ControlObjectClient control = ControlObjectClient_create(select_normal_ctl_obj, con);
    strcpy(resp->ctlCommand, "sbo");
    strcpy(resp->object, control_obj);
    strcpy(resp->lastValue, "none");
    strcpy(resp->iecErrorString, "none");
    strcpy(resp->errorString, "none");

    if (control)
    {
        if (ControlObjectClient_select(control))
        {
            ctlVal = createDynamicMMS(rc.typeData, rc.valueNow);
            strcpy(resp->valueNow, rc.valueNow);
            if (ControlObjectClient_operate(control, ctlVal, 0 /* operate now */))
            {
                printf("%s operated successfully\n", select_normal_ctl_obj);
                strcpy(resp->status, "success");
            }
            else
            {
                printf("failed to operate %s!\n", select_normal_ctl_obj);
                strcpy(resp->status, "failed");
                strcpy(resp->errorString, "value not changed");
            }

            MmsValue_delete(ctlVal);
        }
        else
        {
            printf("failed to select %s!\n", select_normal_ctl_obj);
        }

        ControlObjectClient_destroy(control);
    }
    else
    {
        printf("Control object %s not found in server\n", select_normal_ctl_obj);
        strcpy(resp->status, "failed");
        strcpy(resp->errorString, "item not found");
    }
    strcpy(resp->errorString, "value not changed");
}

int IEC61850_control_direct_security_ex(IedConnection con, char *control_obj, ReceiveControl rc, ResponseControl *resp)
{
    MmsValue *ctlVal = NULL;
    // /****************************************
    //  * Direct control with enhanced security
    //  ****************************************/
    // printf("******* Direct control with enhanced security *******\n");
    const char *direct_security_ctl_obj = control_obj; // control_obj;
    // const char *direct_security_ctl_obj_val = "BCUCPLCONTROL1/GBAY1.Loc.stVal";
    strcpy(resp->ctlCommand, "direct");
    strcpy(resp->object, control_obj);
    strcpy(resp->lastValue, "none");
    strcpy(resp->iecErrorString, "none");
    strcpy(resp->errorString, "none");
    ControlObjectClient control = ControlObjectClient_create(direct_security_ctl_obj, con);

    if (control)
    {
        ControlObjectClient_setCommandTerminationHandler(control, commandTerminationHandler, NULL);

        ctlVal = createDynamicMMS(rc.typeData, rc.valueNow);
        strcpy(resp->valueNow, rc.valueNow);
        if (ControlObjectClient_operate(control, ctlVal, 0 /* operate now */))
        {
            printf("%s operated successfully\n", direct_security_ctl_obj);
            strcpy(resp->valueNow, rc.valueNow);
            strcpy(resp->status, "success");
        }
        else
        {
            printf("failed to operate %s\n", direct_security_ctl_obj);
            strcpy(resp->status, "failed");
            strcpy(resp->errorString, "failed");
        }

        MmsValue_delete(ctlVal);

        /* Wait for command termination message */
        Thread_sleep(1000);

        ControlObjectClient_destroy(control);
    }
    else
    {
        printf("Control object %s not found in server\n", direct_security_ctl_obj);
        strcpy(resp->status, "failed");
        strcpy(resp->errorString, "item not found");
    }
    strcpy(resp->timestamp, current_time_str());
}

int IEC61850_control_sbo_security_ex(IedConnection con, char *control_obj, ReceiveControl rc, ResponseControl *resp)
{
    MmsValue *ctlVal = NULL;
    // /***********************************************
    //  * Select before operate with enhanced security
    //  ***********************************************/
    const char *select_security_ctl_obj = control_obj;
    ControlObjectClient control = ControlObjectClient_create(select_security_ctl_obj, con);
    
    strcpy(resp->valueNow, rc.valueNow);
    strcpy(resp->ctlCommand, "sbo");
    strcpy(resp->object, control_obj);
    strcpy(resp->lastValue, "none");
    strcpy(resp->iecErrorString, "none");
    strcpy(resp->errorString, "none");

    if (control)
    {
        ControlObjectClient_setCommandTerminationHandler(control, commandTerminationHandler, NULL);

        ctlVal = createDynamicMMS(rc.typeData, rc.valueNow);
        strcpy(resp->valueNow, rc.valueNow);

        if (ControlObjectClient_selectWithValue(control, ctlVal))
        {

            if (ControlObjectClient_operate(control, ctlVal, 0 /* operate now */))
            {
                printf("%s operated successfully\n", select_security_ctl_obj);
                strcpy(resp->status, "success");
            }
            else
            {
                printf("failed to operate %s!\n", select_security_ctl_obj);
                strcpy(resp->status, "failed");
                strcpy(resp->errorString, "value not changed");
            }
        }
        else
        {
            printf("failed to select %s!\n", select_security_ctl_obj);
            strcpy(resp->status, "failed");
            strcpy(resp->errorString, "value not changed");
        }

        MmsValue_delete(ctlVal);

        Thread_sleep(1000);

        ControlObjectClient_destroy(control);
    }
    else
    {
        printf("Control object %s not found in server\n", select_security_ctl_obj);
        strcpy(resp->status, "failed");
        strcpy(resp->errorString, "item not found");
    }
    strcpy(resp->timestamp, current_time_str());
}

int IEC61850_control_direct_security_exp_ex(IedConnection con, char *control_obj, ReceiveControl rc, ResponseControl *resp)
{
    MmsValue *ctlVal = NULL;
    MmsValue *stVal = NULL;
    // /*********************************************************************
    //  * Direct control with enhanced security (expect CommandTermination-)
    //  *********************************************************************/
    const char *direct_sec_ct_ctl_obj = control_obj;
    strcpy(resp->ctlCommand, "direct");
    strcpy(resp->object, control_obj);
    strcpy(resp->lastValue, "none");
    strcpy(resp->iecErrorString, "none");
    strcpy(resp->errorString, "none");
    ControlObjectClient control = ControlObjectClient_create(direct_sec_ct_ctl_obj, con);

    if (control)
    {
        ControlObjectClient_setCommandTerminationHandler(control, commandTerminationHandler, NULL);
        ctlVal = createDynamicMMS(rc.typeData, rc.valueNow);
        strcpy(resp->valueNow, rc.valueNow);
        if (ControlObjectClient_operate(control, ctlVal, 0 /* operate now */))
        {
            printf("%s operated successfully\n", direct_sec_ct_ctl_obj);
            strcpy(resp->status, "success");
        }
        else
        {
            printf("failed to operate %s\n", direct_sec_ct_ctl_obj);
            strcpy(resp->status, "failed");
            strcpy(resp->errorString, "value not changed");
        }

        MmsValue_delete(ctlVal);
        Thread_sleep(1000);

        ControlObjectClient_destroy(control);
    }
    else
    {
        printf("Control object %s not found in server\n", direct_sec_ct_ctl_obj);
        strcpy(resp->status, "failed");
        strcpy(resp->errorString, "item not found");
    }
    strcpy(resp->timestamp, current_time_str());
}

int IEC61850_control_cancel_ex(IedConnection con, char *control_obj, ResponseControl *resp)
{
    // /*********************************************************************
    //  * Cancel
    //  *********************************************************************/
    const char *cancel_obj = control_obj;
    ControlObjectClient control = ControlObjectClient_create(cancel_obj, con);
    strcpy(resp->ctlCommand, "cancel");
    strcpy(resp->object, control_obj);
    strcpy(resp->valueNow, "none");
    strcpy(resp->lastValue, "none");
    strcpy(resp->iecErrorString, "none");
    strcpy(resp->errorString, "none");
    if (control)
    {
        ControlObjectClient_setCommandTerminationHandler(control, commandTerminationHandler, NULL);

        if (ControlObjectClient_cancel(control /* operate now */))
        {
            printf("%s cancel successfully\n", cancel_obj);
            strcpy(resp->status, "success");
        }
        else
        {
            printf("failed to cancel %s\n", cancel_obj);
            strcpy(resp->status, "failed");
            strcpy(resp->errorString, "object not found");
        }

        Thread_sleep(1000);

        ControlObjectClient_destroy(control);
    }
    else
    {
        printf("Control object %s not found in server\n", cancel_obj);
        strcpy(resp->status, "failed");
        strcpy(resp->errorString, "item not found");
    }
    strcpy(resp->timestamp, current_time_str());
}
////////////////////// CONTROL ??////////////////////////



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


    if (!loadHostConfigs(configFile, &hostConfig)) {
        printf("Failed to load host configuration.\n");
        return 1;
    }
    strncpy(machine_code_, hostConfig.machineCode, sizeof(machine_code_));
    strncpy(id_device_, hostConfig.id_device, sizeof(id_device_));
    IedClientError error;
    IedConnection con = IedConnection_create();
    int port = atoi(hostConfig.port);
    printf("Connecting to %s:%d\n", hostConfig.ip, port);
    IedConnection_connect(con, &error, hostConfig.ip, port);
    snprintf(mqtt_topic_control_response, sizeof(mqtt_topic_control_response), "DMS/%s/IEC61850/%s/control/response", hostConfig.machineCode, hostConfig.id_device);
    snprintf(mqtt_topic_control_request, sizeof(mqtt_topic_control_request), "DMS/%s/IEC61850/%s/control/request", hostConfig.machineCode, hostConfig.id_device);
    
    initMqttClient();



    if (error != IED_ERROR_OK) {
        printf("Connection failed to %s:%d (error %d)\n", hostConfig.ip, port, error);
        IedConnection_destroy(con);
        return 0;
    }

    ReportSession session = {
        .con = con,
        .error = error,
        .ip = hostConfig.ip,
        .tcpPort = port,
        .connected = true,
        .reportCount = hostConfig.reportCount
    };

    for (int j = 0; j < hostConfig.reportCount; j++) {
        printf("Configuring report #%d - Dataset: %s, RCB: %s\n", j,
            hostConfig.reports[j].dataset,
            hostConfig.reports[j].rcb);

        LinkedList dataSetDirectory = IedConnection_getDataSetDirectory(con, &error, hostConfig.reports[j].dataset, NULL);
        ClientDataSet dataSet = IedConnection_readDataSetValues(con, &error, hostConfig.reports[j].dataset, NULL);
        ClientReportControlBlock rcb = IedConnection_getRCBValues(con, &error, hostConfig.reports[j].rcb, NULL);

        if (!rcb || error != IED_ERROR_OK) {
            printf("RCB error for %s\n", hostConfig.reports[j].rcb);
            continue;
        }

        ClientReportControlBlock_setResv(rcb, true);
        ClientReportControlBlock_setTrgOps(rcb, TRG_OPT_DATA_CHANGED | TRG_OPT_QUALITY_CHANGED | TRG_OPT_GI);
        ClientReportControlBlock_setDataSetReference(rcb, hostConfig.reports[j].dataset);
        ClientReportControlBlock_setRptEna(rcb, true);
        ClientReportControlBlock_setGI(rcb, true);

        IedConnection_installReportHandler(con, hostConfig.reports[j].dataset,
            ClientReportControlBlock_getRptId(rcb),
            reportCallbackFunction, dataSetDirectory);

        IedConnection_setRCBValues(con, &error, rcb, RCB_ELEMENT_RPT_ENA | RCB_ELEMENT_GI, true);

        // Store in session
        session.rcbList[j] = rcb;
        session.dataSetDirectoryList[j] = dataSetDirectory;
        session.clientDataSetList[j] = dataSet;
        session.datasetList[j] = hostConfig.reports[j].dataset;
        session.rcbNameList[j] = hostConfig.reports[j].rcb;
    }

    sessions[sessionCount++] = session;


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
    TopicArrived = false;
    while (running) {
        unsigned long currentTime = millis();
        // printf("Current time: %lu\n", currentTime);

        if (currentTime - lastTime >= interval) {
            lastTime = currentTime;
            // printf("sessioncount: %d\n", sessionCount);
            for (int i = 0; i < sessionCount; i++) {
                ReportSession* session = &sessions[i];
                processMessages(session->con, mqttClient); 

                IedConnectionState state = IedConnection_getState(session->con);
                if (state != IED_STATE_CONNECTED) {
                    printf("Disconnected from %s. Reconnecting...\n", session->ip);
                    session->connected = false;

                    IedConnection_destroy(session->con);
                    session->con = IedConnection_create();
                    IedConnection_connect(session->con, &session->error, session->ip, session->tcpPort);

                    if (session->error != IED_ERROR_OK) {
                        printf("Reconnect failed to %s:%d\n", session->ip, session->tcpPort);
                        continue;
                    }

                    session->connected = true;

                    for (int j = 0; j < session->reportCount; j++) {
                        ClientReportControlBlock_setResv(session->rcbList[j], true);
                        ClientReportControlBlock_setTrgOps(session->rcbList[j], TRG_OPT_DATA_CHANGED | TRG_OPT_QUALITY_CHANGED | TRG_OPT_GI);
                        ClientReportControlBlock_setDataSetReference(session->rcbList[j], session->datasetList[j]);
                        ClientReportControlBlock_setRptEna(session->rcbList[j], true);
                        ClientReportControlBlock_setGI(session->rcbList[j], true);

                        IedConnection_installReportHandler(session->con,
                            session->datasetList[j],
                            ClientReportControlBlock_getRptId(session->rcbList[j]),
                            reportCallbackFunction,
                            session->dataSetDirectoryList[j]);

                        IedConnection_setRCBValues(session->con, &session->error,
                            session->rcbList[j],
                            RCB_ELEMENT_RPT_ENA | RCB_ELEMENT_GI, true);
                    }

                    printf("Reconnected to %s\n", session->ip);
                }

                // Publish MQTT message for this session (can be per dataset/RCB as needed)
                MQTTClient_message pubmsg = MQTTClient_message_initializer;
                char tesSend[100] = "test";
                pubmsg.payload = tesSend;
                pubmsg.payloadlen = (int)strlen(tesSend);
                pubmsg.qos = MQTT_QOS;
                pubmsg.retained = 0;

                MQTTClient_deliveryToken token;
                int rc = MQTTClient_publishMessage(mqttClient, "topic", &pubmsg, &token);
                if (rc == MQTTCLIENT_SUCCESS) {
                    MQTTClient_waitForCompletion(mqttClient, token, MQTT_TIMEOUT);
                    printf("MQTT published for %s\n", session->ip);
                } else {
                    printf("MQTT publish failed: %d\n", rc);
                    initMqttClient();
                }
            }
        }

        // Prevent CPU overload
        usleep(100000); // sleep 1ms
    }


    for (int i = 0; i < sessionCount; i++) {
        ReportSession* session = &sessions[i];
        printf("current time ms: %s\n", current_time_iso8601_ms_local());

        // Cleanup each RCB and related resources
        for (int j = 0; j < session->reportCount; j++) {
            // if (session->rcbList[j]) {
            //     ClientReportControlBlock_setRptEna(session->rcbList[j], false);
            //     IedConnection_setRCBValues(session->con, NULL, session->rcbList[j], RCB_ELEMENT_RPT_ENA, true);
            //     ClientReportControlBlock_destroy(session->rcbList[j]);
            // }

            if (session->clientDataSetList[j])
                ClientDataSet_destroy(session->clientDataSetList[j]);

            if (session->dataSetDirectoryList[j])
                LinkedList_destroy(session->dataSetDirectoryList[j]);
        }

        // Close and destroy the connection
        if (session->con) {
            IedConnection_close(session->con);
            IedConnection_destroy(session->con);
        }
    }

    MQTTClient_disconnect(mqttClient, MQTT_TIMEOUT);
    MQTTClient_destroy(&mqttClient);
    printf("Client example reporting finished.\n");

    return 0;
}
