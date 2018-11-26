/*
 *  MQTT RGB LED example
 *
 *  It connects to an MQTT server then does some stuff
 */

#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// Update these with values suitable for your network.
byte mac[] = { 0xDE, 0xED, 0xBA, 0xFE, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 50);
IPAddress server(192, 168, 1, 23);

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

/* RGB LED Enum */
static const int R = 0;
static const int G = 1;
static const int B = 2;

// The Current Color Variable
int currentcolor[3] = { 0, 0, 0 };

//--------------------------------------------------------------------------------------------------
/* Debug functions */

// void debug_topic(char* topic)
// {
//     Serial.print("    Topic: ");
//     Serial.println(topic);
// }

// void debug_payload(byte* payload, unsigned int length)
// {
//     Serial.print("    Payload: ");
//     for (int i = 0; i < length; i++) {
//         Serial.print((char)payload[i]);
//     }
//     Serial.println();
// }

// void debug_buffer(char* buffer)
// {
//     Serial.print("    Buffer:  ");
//     Serial.print(buffer);
//     Serial.println();
// }

//--------------------------------------------------------------------------------------------------
/* RGB functions */

// Sets an RGB color
void setRGB() 
{
    // Default is white
    int color[3] = { 200, 100, 25 };

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

// Overloaded setRGB() method
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
//--------------------------------------------------------------------------------------------------
/* Helper functions */

// Toggle the pin
void togglePin(int pin)
{
    digitalWrite(pin, !digitalRead(pin));
}

// Toggle the pin
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

void handleRGBTopic(char* data) 
{
    // The json buffer 
    StaticJsonBuffer<256> jsonBuffer;

    // Parse the json object
    JsonObject& root = jsonBuffer.parseObject(data);

    if (!root.success()) {
        Serial.println("parseObject() failed");
    }

    // The current color to fade from
    int oldcolor[3];
    oldcolor[R] = currentcolor[R];
    oldcolor[G] = currentcolor[G];
    oldcolor[B] = currentcolor[B];

    // The new color to fade to
    int newcolor[3];
    newcolor[R] = root["r"].as<int>();
    newcolor[G] = root["g"].as<int>();
    newcolor[B] = root["b"].as<int>();

    // The speed at which to change the colors
    int time = root["time"].as<int>(); 

    if (!time) {
        // If there is no "time" in the json string just set the color
        setRGB(newcolor);
    } else {
        // Else, fade from the old color to the new color
        fade(oldcolor, newcolor, time);
    }
}

//--------------------------------------------------------------------------------------------------
/* MQTT functions */

// The PubSubClient callback function 
void callback(char* topic, byte* payload, unsigned int length) 
{
    //Serial.println("Message arrived!");
    //debug_topic(topic);
    //debug_payload(payload, length);

    char buffer[100]; // payload buffer
    memset(buffer, 0, sizeof(buffer)); // empty the buffer

    for (int i=0; i<length; i++) {
        buffer[i] = (char)payload[i];
    }

    //debug_buffer(buffer);
    
    if (strcmp(topic, "arduino/mega/pwr") == 0) {
        
        togglePin(PWR_PIN, buffer);

    } else if (strcmp(topic, "arduino/mega/rgb") == 0) {

        handleRGBTopic(buffer);

    } else if (strcmp(topic, "arduino/mega/tw1") == 0) {
            
        togglePin(TW1, buffer);
        
    } else if (strcmp(topic, "arduino/mega/tw2") == 0) {
            
        togglePin(TW2, buffer);

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
            client.subscribe("arduino/mega/pwr");
            client.subscribe("arduino/mega/rgb");
            client.subscribe("arduino/mega/tw1");
            client.subscribe("arduino/mega/tw2");
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

    // Set the RGB LED Pins
    pinMode(RED_PIN, OUTPUT);
    pinMode(GREEN_PIN, OUTPUT);
    pinMode(BLUE_PIN, OUTPUT);

    // Set the default values for the twinkle lights
    pinMode(TW1, OUTPUT);
    digitalWrite(TW1, LOW);
    pinMode(TW2, OUTPUT);
    digitalWrite(TW2, LOW);

    // Set the default values for the powers switch 
    pinMode(PWR_PIN, OUTPUT);
    digitalWrite(PWR_PIN, LOW);

    // Set the default values for the the RGB lights
    setRGB();
}

void loop()
{
    if (!client.connected()) {
        reconnect();
    }
    client.loop();
}
