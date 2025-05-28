#ifndef COMMS_H
#define COMMS_H

#include <MQTTClient.h> 

// MQTT related constants
#define MQTT_BROKER_ADDRESS   "tcp://203.194.112.238:1883"
#define MQTT_TOPIC            "dms_C/IEC61850/SRGN_1Measurements/PriFouMMXU1.Hz.mag.f/"
#define MQTT_CLIENT_ID        "IECClient"
#define MQTT_QOS              1
#define MQTT_TIMEOUT          10000L

// Function declaration for MQTT send
void send_to_mqtt(const char *message);

#endif // COMMS_H