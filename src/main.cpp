#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <RCSwitch.h>
#include <Wire.h>
#include "Adafruit_HDC1000.h"
// Update these with values suitable for your network.
#include "../include/settings.h"

#ifdef TLS
WiFiClientSecure espClient;
static unsigned int const mqttPort = 8883;
#else
WiFiClient espClient;
static unsigned int const mqttPort = 1883;
#endif

PubSubClient client(espClient);
RCSwitch mySwitch = RCSwitch();
Adafruit_HDC1000 hdc = Adafruit_HDC1000();
const long interval = 60000; // 1 minute
unsigned long previousMillis = 0;
unsigned long currentMillis = millis();
char stgFromFloat[10];

// can be changed in platformio.ini
#ifdef TC_SWITCH_PIN
static uint8_t const tcSwitchPin = TC_SWITCH_PIN;
#else
static uint8_t const tcSwitchPin = 14;
#endif
#ifdef RC_SWITCH_PIN
static uint8_t const rcSwitchPin = RC_SWITCH_PIN;
#else
static uint8_t const rcSwitchPin = 12;
#endif

static uint8_t const sdaPin = 2;
static uint8_t const sclPin = 0;

void callback(char *topic, byte *payload, unsigned int length)
{
#ifdef DEBUG
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
#endif
    char message[length];
    for (unsigned int i = 0; i < length; i++)
    {
#ifdef DEBUG
        Serial.print((char)payload[i]);
#endif
        message[i] = (char)payload[i];
    }
#ifdef DEBUG
    Serial.println();
#endif

    if (strcmp(topic, lights433topic.c_str()) == 0)
    {
        mySwitch.send(message);
    }
}

void reconnect()
{
    // Loop until we're reconnected
    while (!client.connected())
    {
#ifdef DEBUG
        Serial.print("Attempting MQTT connection...");
#endif
        // Attempt to connect
        // client.connect(clientId.c_str(), mqttUser.c_str(), mqttPassword.c_str()); // User Auth
        if (client.connect(clientId.c_str()))
        {
#ifdef DEBUG
            Serial.println("connected");
#endif
            // Once connected, publish an announcement...
            // client.publish("outTopic", "hello world");
            // ... and resubscribe
            client.subscribe(lights433topic.c_str());
        }
        else
        {
#ifdef DEBUG
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
#endif
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}

void reconnectWiFi()
{
// We start by connecting to a WiFi network
#ifdef DEBUG
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);
#endif
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
#ifdef DEBUG
        Serial.print(".");
#endif
    }

#ifdef DEBUG
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.print("IPv4 address: ");
    Serial.println(WiFi.localIP());
    Serial.print("MAC address: ");
    Serial.println(WiFi.macAddress());
#endif
}

void setup()
{
    pinMode(internalLED, OUTPUT); // Initialize the BUILTIN_LED pin as an output
    Serial.begin(115200);

    Wire.begin(sdaPin, sclPin);
    // Start sensor
    if (!hdc.begin())
    {
#ifdef DEBUG
        Serial.println("Couldn't find sensor!");
#endif
        // TODO: Maybe a mqtt publish with error that sensor could not be started
    }

    delay(10);
// We start by connecting to a WiFi network
#ifdef DEBUG
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);
#endif

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
#ifdef DEBUG
        Serial.print(".");
#endif
    }

#ifdef DEBUG
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.print("IPv4 address: ");
    Serial.println(WiFi.localIP());
    Serial.print("MAC address: ");
    Serial.println(WiFi.macAddress());
#endif

    client.setServer(server, mqttPort);
    client.setCallback(callback);

    mySwitch.enableTransmit(tcSwitchPin);
    mySwitch.setPulseLength(286);

    // Turn the internal LED off by making the voltage HIGH
    digitalWrite(internalLED, HIGH);
}

void loop()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        reconnectWiFi();
    }
    else if (!client.connected())
    {
        reconnect();
    }

    currentMillis = millis();

    if ((currentMillis - previousMillis >= (interval * 5)) || (previousMillis == 0)) // interval is 1 minute. we want to publish every 5 minutes
    {
        previousMillis = currentMillis;
#ifdef DEBUG
        Serial.print("Temp: ");
        Serial.print(hdc.readTemperature());
        Serial.print("\t\tHum: ");
        Serial.println(hdc.readHumidity());
#endif
        dtostrf(hdc.readTemperature(), 4, 2, stgFromFloat);
        client.publish("the-verse/floor/temperature", stgFromFloat, true); // Send as retained message so last vlaue can be accesed by subscriber
        dtostrf(hdc.readHumidity(), 4, 2, stgFromFloat);
        client.publish("the-verse/floor/humidity", stgFromFloat, true); // Send as retained message so last vlaue can be accesed by subscriber
    }

    client.loop();
}
