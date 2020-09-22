/*
 *  MQTT RGB LED example
 *
 *  It connects to an MQTT server then does some stuff
 */

#include "config.h"
#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

EthernetClient ethClient;
PubSubClient client(ethClient);

/* Power Switch pins */
static const int PWR_PIN = 30;

/* RGB Pins */
static const int RED_PIN = 5;
static const int GREEN_PIN = 7;
static const int BLUE_PIN = 6;

/* Twinkle Light Pins */
static const int TW1 = 44;
static const int TW2 = 45;
static const int TW3 = 46;

/* RGB LED Enum */
static const int R = 0;
static const int G = 1;
static const int B = 2;

// The Current Color Variable
int currentcolor[3] = { 0, 0, 0 };

//--------------------------------------------------------------------------------------------------
/* RGB functions */

void setRGB(int color[3])
{
    if (color[R] < 0) { color[R] = 0; }
    if (color[R] > 255) { color[R] = 255; }
    if (color[G] < 0) { color[G] = 0; }
    if (color[G] > 255) { color[G] = 255; }
    if (color[B] < 0) { color[B] = 0; }
    if (color[B] > 255) { color[B] = 255; }

    analogWrite(RED_PIN, color[R]);
    analogWrite(GREEN_PIN, color[G]);
    analogWrite(BLUE_PIN, color[B]);

    currentcolor[R] = color[R];
    currentcolor[G] = color[G];
    currentcolor[B] = color[B];
}

// Controls a smooth lighting fade
void fade(int startcolor[], int endcolor[], int fadeTime)
{
    int color[3];

    for (int t = 0; t < fadeTime; t++) {
        color[R] = map(t, 0, fadeTime, startcolor[R], endcolor[R]);
        color[G] = map(t, 0, fadeTime, startcolor[G], endcolor[G]);
        color[B] = map(t, 0, fadeTime, startcolor[B], endcolor[B]);

        setRGB(color);
        delay(1);
    }

    setRGB(endcolor);
    delay(1);
}

void handleRGBTopic(char *data)
{
    // The json buffer
    StaticJsonDocument<256> doc;
    // Parse the json object
    DeserializationError error = deserializeJson(doc, data);
    // Handle error
    if (error) {
        Serial.println("DeserializationError!");
        Serial.print("Error: ");
        Serial.println(error.c_str());
        return;
    }

    // The current color to fade from
    int oldcolor[3];
    oldcolor[R] = currentcolor[R];
    oldcolor[G] = currentcolor[G];
    oldcolor[B] = currentcolor[B];

    // The new color to fade to
    int newcolor[3];
    newcolor[R] = doc["r"];
    newcolor[G] = doc["g"];
    newcolor[B] = doc["b"];

    // The speed at which to change the colors
    int time = doc["time"];

    if (!time) {
        // If there is no "time" in the json string just set the color
        setRGB(newcolor);
    } else {
        // Else, fade from the old color to the new color
        fade(oldcolor, newcolor, time);
    }
}

//--------------------------------------------------------------------------------------------------
/* Functions */

void togglePin(int pin)
{
    digitalWrite(pin, !digitalRead(pin));
}

void togglePin(int pin, char* data)
{
    if (
        strcmp(data, "1") == 0
    ||  strcmp(data, "on") == 0
    ||  strcmp(data, "ON") == 0
    ) {
        digitalWrite(pin, HIGH);
    } else if (
        strcmp(data, "0") == 0
    ||  strcmp(data, "off") == 0
    ||  strcmp(data, "OFF") == 0
    ) {
        digitalWrite(pin, LOW);
    }
}

void publishRGBStatus()
{
    char output[128];
    const int capacity = JSON_OBJECT_SIZE(3);
    
    StaticJsonDocument<capacity> doc;

    doc["r"] = currentcolor[R];
    doc["g"] = currentcolor[G];
    doc["b"] = currentcolor[B];

    serializeJson(doc, output, sizeof(output));

    client.publish(PUB_RGB, output);
}

void publishPinStatus(char *topic_postfix, int pin)
{
    char topic[30] = PUB_BASE;
    strcat(topic, topic_postfix);
    
    if (digitalRead(pin)) {
        client.publish(topic, "ON");
    } else {
        client.publish(topic, "OFF");
    }
}

void publishDeviceStatus()
{
    publishRGBStatus();
    publishPinStatus("pwr", PWR_PIN);
    publishPinStatus("tw1", TW1);
    publishPinStatus("tw2", TW2);
    publishPinStatus("tw3", TW3);
}

//--------------------------------------------------------------------------------------------------
/* MQTT functions */

// The PubSubClient callback function
void callback(char* topic, byte* payload, unsigned int length)
{
    char buffer[128];                     // Deifne a payload buffer
    memset(buffer, '\0', sizeof(buffer)); // Then makesure it's empty

    for (int i=0; i<length; i++) {
        buffer[i] = (char)payload[i];
    }

    // Handle the topic
    if (strcmp(topic, SUB_RGB) == 0) {

        handleRGBTopic(buffer);

    } else if (strcmp(topic, SUB_PWR) == 0) {

        togglePin(PWR_PIN, buffer);
        publishPinStatus("pwr", PWR_PIN);

    } else if (strcmp(topic, SUB_TW1) == 0) {

        togglePin(TW1);
        publishPinStatus("tw1", TW1);

    } else if (strcmp(topic, SUB_TW2) == 0) {

        togglePin(TW2);
        publishPinStatus("tw2", TW2);

    } else if (strcmp(topic, SUB_TW3) == 0) {

        togglePin(TW3);
        publishPinStatus("tw3", TW3);

    } else if (strcmp(topic, SUB_STATUS) == 0) {

        publishDeviceStatus();

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
        if (client.connect("ArduinoMegaLED")) {
            Serial.println("connected"); // Once connected, publish an announcement...
            client.publish(PUB_CONFIRM, "ArduinoMegaLED has connected to the broker");
            // ... and resubscribe
            client.subscribe(SUB_RGB);
            client.subscribe(SUB_PWR);
            client.subscribe(SUB_TW1);
            client.subscribe(SUB_TW2);
            client.subscribe(SUB_TW3);
            client.subscribe(SUB_STATUS);
        } else {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}

//--------------------------------------------------------------------------------------------------

void setup()
{
    Serial.begin(115200);

    client.setServer(MQTT_BROKER, MQTT_PORT);
    client.setCallback(callback);

    if (Ethernet.begin(mac) == 0) {
        Serial.println("Failed to configure Ethernet using DHCP, using Static Mode");
        Ethernet.begin(mac, ip, dns, gateway, subnet);
    }

    Serial.print("My IP address: ");
    Serial.println(Ethernet.localIP());

    delay(1500); // Allow the hardware to sort itself out

    Serial.println("Start!");

    // Set the RGB LED Pins
    pinMode(RED_PIN, OUTPUT);
    pinMode(GREEN_PIN, OUTPUT);
    pinMode(BLUE_PIN, OUTPUT);

    // Set the default values for the twinkle lights
    pinMode(TW1, OUTPUT);
    digitalWrite(TW1, LOW);
    pinMode(TW2, OUTPUT);
    digitalWrite(TW2, LOW);
    pinMode(TW3, OUTPUT);
    digitalWrite(TW3, LOW);

    // Set the default values for the powers switch
    pinMode(PWR_PIN, OUTPUT);
    digitalWrite(PWR_PIN, LOW);
}

void loop()
{
    if (!client.connected()) {
        reconnect();
    }
    client.loop();
}
