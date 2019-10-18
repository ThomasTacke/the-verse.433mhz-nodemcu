#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <RCSwitch.h>
// Update these with values suitable for your network.
#include "../lib/settings.h"

#ifdef TLS
WiFiClientSecure espClient;
static unsigned int const mqttPort = 8883;
#else
WiFiClient espClient;
static unsigned int const mqttPort = 1883;
#endif

PubSubClient client(espClient);
RCSwitch mySwitch = RCSwitch();

// can be changed in platformio.ini
#ifdef RC_SWITCH_PIN 
static uint8_t const rcSwitchPin = RC_SWITCH_PIN;
#else
static uint8_t const rcSwitchPin = 2; // default value
#endif
// static char const c1Off[] = "010111011101010000000011";
// static char const c1On[] = "010111011101010000001100";
// static char const c2Off[] = "010111010111010000000011";
// static char const c2On[] = "010111010111010000001100";
// static char const c3Off[] = "010111010101110000000011";
// static char const c3On[] = "010111010101110000001100";

void callback(char* topic, byte* payload, unsigned int length) {
  #ifdef DEBUG
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  #endif
  char message[length];
  for (unsigned int i = 0; i < length; i++) {
    #ifdef DEBUG
    Serial.print((char)payload[i]);
    #endif
    message[i] = (char)payload[i];
  }
  #ifdef DEBUG
  Serial.println();
  #endif

  if(strcmp(topic, lights433topic.c_str()) == 0) {
    mySwitch.send(message);
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    #ifdef DEBUG
    Serial.print("Attempting MQTT connection...");
    #endif
    // Attempt to connect
    // client.connect(clientId.c_str(), mqttUser.c_str(), mqttPassword.c_str()); // User Auth
    if (client.connect(clientId.c_str())) {
      #ifdef DEBUG
      Serial.println("connected");
      #endif
      // Once connected, publish an announcement...
      // client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe(lights433topic.c_str());
    } else {
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

void reconnectWiFi() {
  // We start by connecting to a WiFi network
  #ifdef DEBUG
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  #endif
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
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

void setup() {
  pinMode(internalLED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200);
  
  delay(1000);
  // We start by connecting to a WiFi network
  #ifdef DEBUG
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  #endif

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
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

  // Transmitter is connected to NodeMCU Pin ~D3 = 0
  mySwitch.enableTransmit(rcSwitchPin);
  // Optional set pulse length.
  mySwitch.setPulseLength(286);

  // Turn the internal LED off by making the voltage HIGH
  digitalWrite(internalLED, HIGH);  
}

void loop() {

  if (WiFi.status() != WL_CONNECTED) {
    reconnectWiFi();
  } else if (!client.connected()) {
    reconnect();
  }
  client.loop();
}
