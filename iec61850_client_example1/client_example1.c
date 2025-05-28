#include "iec61850_client.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <mariadb/mysql.h>
#include <MQTTClient.h>
#include <time.h>
#include <unistd.h>

#define MQTT_BROKER_ADDRESS   "tcp://203.194.112.238:1883"
#define MQTT_TOPIC            "dms_C/IEC61850/SRGN_1Measurements/PriFouMMXU1.Hz.mag.f/"
#define MQTT_CLIENT_ID        "IECClient"
#define MQTT_QOS              1
#define MQTT_TIMEOUT          10000L

#define DB_HOST               "localhost"
#define DB_USER               "global"
#define DB_PASSWORD           "12345678"
#define DB_NAME               "dms"
#define DB_TABLE              "relay"

// Function to send data to the MQTT broker
void send_to_mqtt(const char *message) {
    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;

    // Create MQTT client
    MQTTClient_create(&client, MQTT_BROKER_ADDRESS, MQTT_CLIENT_ID, MQTTCLIENT_PERSISTENCE_NONE, NULL);

    // Connect to the MQTT broker
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    MQTTClient_connect(client, &conn_opts);

    // Prepare message to send
    pubmsg.payload = (void *)message;
    pubmsg.payloadlen = (int)strlen(message);
    pubmsg.qos = MQTT_QOS;
    pubmsg.retained = 0;

    // Publish the message
    MQTTClient_publishMessage(client, MQTT_TOPIC, &pubmsg, &token);
    printf("Message published to MQTT: %s\n", message);

    // Wait for message delivery to complete
    MQTTClient_waitForCompletion(client, token, MQTT_TIMEOUT);

    // Disconnect and destroy client
    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
}

// Function to store data in the database
void store_in_db(const char* value) {
    MYSQL *conn;
    MYSQL_RES *res;
    MYSQL_ROW row;
    char query[1024];

    // Initialize MySQL connection
    conn = mysql_init(NULL);
    if (conn == NULL) {
        fprintf(stderr, "mysql_init() failed\n");
        return;
    }

    // Connect to MySQL database
    if (mysql_real_connect(conn, DB_HOST, DB_USER, DB_PASSWORD, DB_NAME, 0, NULL, 0) == NULL) {
        fprintf(stderr, "mysql_real_connect() failed\n");
        mysql_close(conn);
        return;
    }

    float float_value = atof(value);

    // Prepare SQL query
    snprintf(query, sizeof(query), "INSERT INTO %s (x_float) VALUES ('%f')", DB_TABLE, float_value);

    // Execute query
    if (mysql_query(conn, query)) {
        fprintf(stderr, "INSERT failed. Error: %s\n", mysql_error(conn));
    } else {
        printf("Data inserted into database\n");
    }

    // Close the connection
    mysql_close(conn);
}

// Function to get a sample value using IEC 61850 protocol
void get_iec_sample_value(IedConnection con) {
    IedClientError error;
    char text1[]  =  "SRGN_1Measurements/PriFouMMXU1.Hz.mag.f";
    char text2[]  =  "SRGN_1Measurements/LLN0.t";
    MmsValue* value = IedConnection_readObject(con, &error, text1, 1);
    
    if (value != NULL) {
        if (MmsValue_getType(value) == MMS_FLOAT) {
            float fval = MmsValue_toFloat(value);
            printf("Read float value: %f\n", fval);
            
            // Send the value to MQTT
            char message[256];
            snprintf(message, sizeof(message), "%f", fval);
            send_to_mqtt(message);

            // Store the value in the database
            snprintf(message, sizeof(message), "%f", fval);
            store_in_db(message);
        }
        else {
            printf("Failed to read value (error code: %i)\n", MmsValue_getDataAccessError(value));
        }
        MmsValue_delete(value);
    }
    else {
        printf("Failed to read object\n");
    }
}

int main(int argc, char** argv) {
    for(;;){
        char* hostname;
        int tcpPort = 102;
        const char* localIp = NULL;
        int localTcpPort = -1;

        if (argc > 1)
            hostname = argv[1];
        else
            hostname = "localhost";

        if (argc > 2)
            tcpPort = atoi(argv[2]);

        if (argc > 3)
            localIp = argv[3];

        if (argc > 4)
            localTcpPort = atoi(argv[4]);

        IedClientError error;

        IedConnection con = IedConnection_create();

        /* Optional bind to local IP address/interface */
        if (localIp) {
            IedConnection_setLocalAddress(con, localIp, localTcpPort);
            printf("Bound to Local Address: %s:%i\n", localIp, localTcpPort);
        }

        IedConnection_connect(con, &error, hostname, tcpPort);
        printf("Connecting to %s:%i\n", hostname, tcpPort);

        if (error == IED_ERROR_OK) {
            printf("Connected\n");
            get_iec_sample_value(con);
        }1
        else {
            printf("Failed to connect to %s:%i\n", hostname, tcpPort);
        }

        IedConnection_destroy(con);
        sleep(5);
    }

    return 0;
}
