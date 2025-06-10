import paho.mqtt.client as mqtt
import json
import time

# MQTT Configuration
BROKER = "localhost"
PORT = 1883
TOPIC = "DMS/1/control/request"
CLIENT_ID = "mqtt_iec_publisher"

# Create client
client = mqtt.Client(CLIENT_ID)

# Connect to broker
client.connect(BROKER, PORT, keepalive=60)

# Loop publish every 5 seconds
value_now = False
try:
    while True:
        # Build message
        message = {
            "object": "TEMPLATECON/RBGGIO1.SPCSO01",
            "valueNow": value_now,
            "lastValue": not value_now,
            "typeData": "boolean",
            "ctlCommand": "direct",
            "interlocking": False,
            "synchrocheck": False,
            "testmode": False,
            "timestamp": int(time.time())
        }

        # Convert to JSON string
        payload = json.dumps(message)

        # Publish
        result = client.publish(TOPIC, payload)

        # Check result
        if result.rc == mqtt.MQTT_ERR_SUCCESS:
            print(f"✅ Published to {TOPIC}: valueNow={value_now}")
        else:
            print("❌ Failed to publish message")

        # Alternate valueNow
        value_now = not value_now

        # Wait 5 seconds
        time.sleep(2.5)

except KeyboardInterrupt:
    print("⛔ Stopped by user")

finally:
    client.disconnect()
