#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <EEPROM.h>


const char* ssid = "ATTsn2wI3Q";
const char* password = "gre6qjtxpmyw";
const char* mqtt_server = "192.168.1.101";

// configure static IP
IPAddress staticIP(192,168,1,102);
IPAddress gateway(192,168,1,254);
IPAddress subnet(255,255,255,0);


#define DHT_In D5
#define DHT_Power D7
#define Sleep_Mode D1


WiFiClient espClient;
PubSubClient client(espClient);
DHT dht(DHT_In, DHT22);

unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];


int MQTT_retries = 0;
int wifi_retries = 0;
float t = 0.0;
float h = 0.0;
int poll_counter = 0;
boolean sleep_mode = false;
byte EEPROM_val = 0;
byte byte1 = 7;

void blink(){

//digitalWrite(LED_BUILTIN, LOW);
delay(100);
//digitalWrite(LED_BUILTIN, HIGH);

}


void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    blink();
  } 
  else if ((char)payload[0] == 't'){
    //digitalWrite(LED_BUILTIN, LOW);
  }
  else if ((char)payload[0] == 'f'){
    //digitalWrite(LED_BUILTIN, HIGH);
  }
  else if ((char)payload[0] == 's'){
    sleep_mode = true;
  }
  else if ((char)payload[0] == 'w'){
    sleep_mode = false;
  }
  else {
    // something for later
  }

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(),"pi","espmaster6969")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      snprintf (msg, MSG_BUFFER_SIZE, "{\"MQTT SVR\" : \"Connected with %d MQTT attempts\"}", MQTT_retries);
      client.publish("ESP/chatter", msg);
      // ... and resubscribe
      client.subscribe("ESP/CMD");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      MQTT_retries++;
      // Wait 5 seconds before retrying
      delay(2500);
    }
  }
}
void setup() {
  
Serial.begin(115200);
delay(100);
// We start by connecting to a WiFi network
Serial.println();
Serial.print("Connecting to ");
Serial.println(ssid);

WiFi.mode(WIFI_STA);
WiFi.begin(ssid, password);
WiFi.config(staticIP, gateway, subnet);

while (WiFi.status() != WL_CONNECTED) {
  delay(500);
  Serial.print(".");
  //track how many times you're connecting to Wifi
  wifi_retries++;
}

Serial.println("");
Serial.println("WiFi connected");
Serial.println("IP address: ");
Serial.println(WiFi.localIP());

//MQTT connection to raaspi broker 
client.setServer(mqtt_server, 8883);
client.setCallback(callback);

//power sensor using GPIO to conserve power
pinMode(DHT_Power, OUTPUT); 
pinMode(Sleep_Mode, INPUT);


digitalWrite(DHT_Power,HIGH);
delay(1);
dht.begin();
delay(1000);

}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
 
  client.loop();
  unsigned long now = millis();
  
  if (now - lastMsg > 2100) {
    
    lastMsg = now;
    
    if (poll_counter > 0 && digitalRead(Sleep_Mode) == HIGH){
        //client.publish("ESP/chatter", "I'm awake, but going to sleep for 15min");
        digitalWrite(DHT_Power, LOW);
        //ESP.deepSleep(60e6);
        ESP.deepSleep(1800e6);
    }
    
    t = dht.readTemperature(true);
    // delay(2050); not sure if this is needed
    h = dht.readHumidity();
    delay(1);
    Serial.println("Temperature in F:");  
    Serial.println(t);
    Serial.println("% Humidity:");  
    Serial.println(h);
    snprintf (msg, MSG_BUFFER_SIZE, "{\"temp\":%0.1lf,\"humidity\":%0.2lf,\"wifi\":%d }", t, h, wifi_retries);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish("ESP/chatter", msg);

    poll_counter++;
  }
}

