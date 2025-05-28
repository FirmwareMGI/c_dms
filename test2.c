// test 2

#include "src/comms.h"
#include "src/database.h"
#include "src/iec_utils.h"
#include "src/list_utils.h"
#include "src/sockets.h"
#include <linked_list.h>
#include <stdio.h>              
#include <stdlib.h>             
#include <string.h>             
#include <unistd.h>             
#include <mariadb/mysql.h>      
#include <MQTTClient.h>         
#include <time.h>               

//========================list stuff=========================
// char *alias_name = [];
// char *rack_location = [];
// char *alias_monit = [];
// char *ipserver = [];
// char *hostname = [];
// char *tcpport = [];
// char *error = [];
// char *con = [];
// char *model = [];
// char *object_read = [];
// char *object_FC = [];
// char *file_list = [];
// char *type_relay = [];
// char *disturbanceType = [];
// char *iecfolder = [];

// string_list *alias_name;
            // rack_location, alias, monit, 
            // ipserver, hostname, tcpport, error, con, 
            // model, object_read, object_fc, file_list, 
            // type_relay, name_alias, disturbanceType, iecfolder;

// list_init(alias_name);
// init(&rack_location);
// init(&alias);
// init(&monit);
// init(&ipserver);
// init(&hostname);
// init(&tcpport);
// init(&error);
// init(&con);
// init(&model);
// init(&object_read);
// init(&object_fc);
// init(&file_list);
// init(&type_relay);
// init(&name_alias);
// init(&disturbanceType);
// init(&iecfolder);
//============================================================

// linked list
void addStringsToList(LinkedList myList) {
    // Add some strings to the linked list
    char* item1 = malloc(20 * sizeof(char));  // Allocate memory for item1
    strcpy(item1, "Hello\n");
    printf(item1);

    char* item2 = malloc(20 * sizeof(char));  // Allocate memory for item2
    strcpy(item2, "World\n");
    printf(item2);

    char* item3 = malloc(20 * sizeof(char));  // Allocate memory for item3
    strcpy(item3, "99\n");
    printf(item3);

    char* item4 = malloc(20 * sizeof(char));  // Allocate memory for item4
    strcpy(item4, "88\n");
    printf(item4);

    // char* item5 = malloc(20 * sizeof(char));  // Allocate memory for item4
    // strcpy(item5, "
    //     {'id': dataDevice[i][\"id_device\"],
    //     'hostname': dataDevice[i][\"ip_address\"]}
    //     ");
    // printf("%s", item5);
    
    // Add items to the linked list
    LinkedList_add(myList, item1);  // Adding "Hello"
    LinkedList_add(myList, item2);  // Adding "World"
    LinkedList_add(myList, item3);  // Adding "99"
    LinkedList_add(myList, item4);  // Adding "88"

    printf("Size of the list: ");
    printf("%d\n", LinkedList_size(myList));
}

// Function to send data to the MQTT broker (Implementation)
// void send_to_mqtt(const char *message) {
//     MQTTClient client;
//     MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
//     MQTTClient_message pubmsg = MQTTClient_message_initializer;
//     MQTTClient_deliveryToken token;

//     // Create MQTT client
//     MQTTClient_create(&client, MQTT_BROKER_ADDRESS, MQTT_CLIENT_ID, MQTTCLIENT_PERSISTENCE_NONE, NULL);

//     // Connect to the MQTT broker
//     conn_opts.keepAliveInterval = 20;
//     conn_opts.cleansession = 1;
//     MQTTClient_connect(client, &conn_opts);

//     // Prepare message to send
//     pubmsg.payload = (void *)message;
//     pubmsg.payloadlen = (int)strlen(message);
//     pubmsg.qos = MQTT_QOS;
//     pubmsg.retained = 0;

//     // Publish the message
//     MQTTClient_publishMessage(client, MQTT_TOPIC, &pubmsg, &token);
//     printf("Message published to MQTT: %s\n", message);

//     // Wait for message delivery to complete
//     MQTTClient_waitForCompletion(client, token, MQTT_TIMEOUT);

//     // Disconnect and destroy client
//     MQTTClient_disconnect(client, 10000);
//     MQTTClient_destroy(&client);
// }

// Function to get a sample value using IEC 61850 protocol (Implementation)
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
            // send_to_mqtt(message);

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
        // linked list
        printf("creating the list...\n");
        LinkedList test_list = LinkedList_create();
        printf("done\n");
        printf("doing some addition...\n");
        addStringsToList(test_list);
        printf("done. destroying...\n");
        printf("done.\n");
        LinkedList_destroy(test_list);

        read_network();
        read_network_mqtt();
        read_device_list();
        read_device_list_by_mode();
        // read_file_dr_fail();
        // read_file_dr_last();
        // read_m_file_iec_active();

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
        }
        else {
            printf("Failed to connect to %s:%i\n", hostname, tcpPort);
        }

        IedConnection_destroy(con);
        sleep(5);
    }

    return 0;
}