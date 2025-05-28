#ifndef COMMS_H
#define COMMS_H

#include "iec61850_client.h"
#include <MQTTClient.h>      // Potentially needed for send_to_mqtt (MQTTClient type)
#include <mariadb/mysql.h>   // Potentially needed for store_in_db (MYSQL type)
#include <stdio.h>           // Potentially needed if function prototypes use stdio types, better move to main.c if not needed in header *Correction: No need in header for prototypes, move to main.c and implementations file.*
#include <stdlib.h>          // Potentially needed for function prototypes, better move to main.c if not needed in header *Correction: No need in header for prototypes, move to main.c and implementations file.*
#include <string.h>          // Potentially needed for function prototypes, better move to main.c if not needed in header *Correction: No need in header for prototypes, move to main.c and implementations file.*
#include <time.h>            // Potentially needed for function prototypes, better move to main.c if not needed in header *Correction: No need in header for prototypes, move to main.c and implementations file.*
#include <unistd.h>          // Potentially needed for function prototypes, better move to main.c if not needed in header *Correction: No need in header for prototypes, move to main.c and implementations file.*

#define MQTT_BROKER_ADDRESS "tcp://203.194.112.238:1883"
#define MQTT_TOPIC "dms_C/IEC61850/SRGN_1Measurements/PriFouMMXU1.Hz.mag.f/"
#define MQTT_CLIENT_ID "IECClient"
#define MQTT_QOS 1
#define MQTT_TIMEOUT 10000L

#define DB_HOST "localhost"
#define DB_USER "global"
#define DB_PASSWORD "12345678"
#define DB_NAME "dms"
#define DB_TABLE "relay"

// Function to send data to the MQTT broker
void send_to_mqtt(const char *message);

// Function to store data in the database
void store_in_db(const char *value);

// Function to get a sample value using IEC 61850 protocol
void get_iec_sample_value(IedConnection con);

#endif // BASE_H