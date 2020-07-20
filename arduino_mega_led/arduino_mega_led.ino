/*
 *  MQTT LED example
 *
 *  It connects to an MQTT server then:
 *      - publishes a confirmation that it has connected 
 *      - subscribes to the topics "tw1" and "tw2
 *          - If the incoming message is "1" it turn the led on
 *          - If the incoming message is "0" it turns the led off 
 */
#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>

// Update these with values suitable for your network.
byte mac[] = { 0xDE, 0xED, 0xBA, 0xFE, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 50);
IPAddress server(192, 168, 1, 23);

EthernetClient ethClient;
PubSubClient client(ethClient);

/* Led variables */
static const int TW1 = 44;
static const int TW2 = 45;

void debug_topic(char* topic)
{
    Serial.print("    Topic: ");
    Serial.println(topic);
}

void debug_payload(byte* payload, unsigned int length)
{
    Serial.print("    Payload: ");
    for (int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
    }
    Serial.println();
}

void debug_buffer(char* buffer)
{
    Serial.print("    Buffer:  ");
    Serial.print(buffer);
    Serial.println();
}

void callback(char* topic, byte* payload, unsigned int length) 
{
    //Serial.println("Message arrived!");
    //debug_topic(topic);
    //debug_payload(payload, length);

    char buff[(length + 1)]; // payload buffer
    memset(buff, '\0', (length + 1));

    for (int i=0; i<length; i++) {
        buff[i] = (char)payload[i];
    }

    //debug_buffer(buff);

    if (strcmp(topic, "tw1") == 0) {
        if (strcmp(buff, "1") == 0) {
            digitalWrite(TW1, HIGH);
        } else if (strcmp(buff, "0") == 0) {
            digitalWrite(TW1, LOW);
        }
    } else if (strcmp(topic, "tw2") == 0) {
        if (strcmp(buff, "1") == 0) {
            digitalWrite(TW2, HIGH);
        } else if (strcmp(buff, "0") == 0) {
            digitalWrite(TW2, LOW);
        }
    } else {
        Serial.print("Could not handle topic: \"");
        Serial.print(topic);
        Serial.println("\"");
    }
}

void reconnect() 
{
    // Loop until we're reconnected
    while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");
        // Attempt to connect
        if (client.connect("arduinoLED")) {
            Serial.println("connected");
            // Once connected, publish an announcement...
            client.publish("confirm", "arduinoLED has connected to the broker");
            // ... and resubscribe
            client.subscribe("tw1");
            client.subscribe("tw2");
        } else {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}

void setup()
{
    Serial.begin(115200);

    client.setServer(server, 1883);
    client.setCallback(callback);

    Ethernet.begin(mac, ip);

    // Allow the hardware to sort itself out
    delay(1500);

    Serial.println("Start!");
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
        Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
        while (true) {
            delay(1); // do nothing, no point running without Ethernet hardware
        }
    }
    
    pinMode(TW1, OUTPUT);
    digitalWrite(TW1, HIGH);
    pinMode(TW2, OUTPUT);
    digitalWrite(TW2, HIGH);
}

void loop()
{
    if (!client.connected()) {
        reconnect();
    }
    client.loop();
}
