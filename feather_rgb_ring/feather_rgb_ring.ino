#include "config.h"
#include "ColorLed.h"
#include "NeoPixelRing.h"
#include <Adafruit_NeoPixel.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>

#define NEO_PIXEL_PIN 12

//==============================================================================
// Globals

/**
 *  Adafruit NeoPixel object
 *
 *  Argument 1 = Number of pixels in NeoPixel strip
 *  Argument 2 = Arduino pin number (most are valid)
 *  Argument 3 = Pixel type flags, add together as needed:
 *      NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
 *      NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
 *      NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
 *      NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
 *      NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
 */
Adafruit_NeoPixel neoPixel(12, NEO_PIXEL_PIN, NEO_GRB + NEO_KHZ800);

/**
 *  My implementation of the neopixel ring in object form.
 */
NeoPixelRing ring(&neoPixel);

/**
 *  ESP8266 WiFiClient class to connect to the MQTT server
 */
WiFiClient client;

/**
 *  MQTT CLient connecting to my home mqtt broker
 */
Adafruit_MQTT_Client mqtt(&client, MQTT_BROKER, MQTT_PORT);

/**
 *  MQTT Client subscrtiptions 
 */
Adafruit_MQTT_Subscribe setColor       = Adafruit_MQTT_Subscribe(&mqtt, SUB_SET_COLOR);
Adafruit_MQTT_Subscribe getColorStatus = Adafruit_MQTT_Subscribe(&mqtt, SUB_GET_COLOR_STATUS);

//==============================================================================
// Functions

void connectToBroker()
{
    if (mqtt.connected()) {
        return;
    }

    Serial.print(F("Connecting to MQTT... "));
    
    int8_t retries = 3;
    int8_t connectionResponse;
    while ((connectionResponse = mqtt.connect()) != 0) {
        Serial.println("");
        Serial.print(F("Error: "));
        Serial.println(mqtt.connectErrorString(connectionResponse));
        Serial.println(F("Retrying MQTT connection in 5 seconds..."));
        mqtt.disconnect();
        delay(5000); // wait 5 seconds
        retries--;

        if (retries == 0) {
            Serial.println("");
            Serial.println(F("Reached max attempts. Please reset the board."));
            while (1);
        }
    }

    Serial.println(F("Success!"));
}

void setColorCallback(char *data, uint16_t len)
{
    if (SUBSCRIPTIONDATALEN < len) {
        Serial.println(F("data is larger than max legnth. Can not parse."));
        return;
    }

    Serial.print("Data: ");
    Serial.println(data);
    Serial.print("Length: ");
    Serial.println(len);
    
    RGB color = {0,0,0};
    const int capacity = JSON_OBJECT_SIZE(4);
    StaticJsonDocument<capacity> doc;
    DeserializationError err = deserializeJson(doc, data);
    
    if (err) {
        Serial.print(F("deserializeJson() failed with code "));
        Serial.println(err.c_str());
        return;
    }

    color.r = doc["r"].as<uint8_t>();
    color.g = doc["g"].as<uint8_t>();
    color.b = doc["b"].as<uint8_t>();
    int time = doc["time"];

    Serial.print(F("r: "));
    Serial.println(color.r);
    Serial.print(F("g: "));
    Serial.println(color.g);
    Serial.print(F("b: "));
    Serial.println(color.b);
    Serial.print(F("time: "));
    Serial.println(time);

    if (time) {
        ring.fadeColor(&color, time);
    } else {
        ring.setColor(&color);
    }
}

void getColorStatusCallback(char *data, uint16_t len)
{
    if (SUBSCRIPTIONDATALEN < len) {
        Serial.println(F("data is larger than max legnth. Can not parse."));
        return;
    }

    Serial.print("Data: ");
    Serial.println(data);
    Serial.print("Length: ");
    Serial.println(len);
    
    char output[SUBSCRIPTIONDATALEN];
    RGB color = ring.getColor();
    const int capacity = JSON_OBJECT_SIZE(3);
    StaticJsonDocument<capacity> doc;

    doc["r"] = color.r;
    doc["g"] = color.g;
    doc["b"] = color.b;

    serializeJson(doc, output, sizeof(output));

    mqtt.publish(PUB_GET_COLOR_STATUS, output);
}

//==============================================================================
// Main

void setup() 
{
    Serial.begin(115200);
    delay(100);

    // init pins
    pinMode(NEO_PIXEL_PIN, OUTPUT);
    digitalWrite(NEO_PIXEL_PIN, LOW);

    // Connect to wifi
    Serial.println("");
    Serial.println("");
    Serial.print("Connecting to ");
    Serial.print(WLAN_SSID);

    WiFi.begin(WLAN_SSID, WLAN_PASS);

    while (WiFi.status() != WL_CONNECTED) {
        delay(750);
        Serial.print(".");
    }

    Serial.println(" Success!");  
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    // Setup Mqtt
    mqtt.unsubscribe(&setColor);
    mqtt.unsubscribe(&getColorStatus);

    setColor.setCallback(setColorCallback);
    mqtt.subscribe(&setColor);
    getColorStatus.setCallback(getColorStatusCallback);
    mqtt.subscribe(&getColorStatus);

    // initialize neopixel
    neoPixel.begin(); // Also initializes the pinmode and state
    ring.off();       // Turn OFF all pixels  
}

void loop() 
{
    connectToBroker();

    mqtt.processPackets(10000);

    // ping the server to keep the mqtt connection alive
    if(!mqtt.ping()) {
        mqtt.disconnect();
    }
}
