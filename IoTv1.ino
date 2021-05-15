/* DHT11 + ThingSpeak + webserver + Telegram group + using millis(no delay) */

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "DHT.h"
#include "ThingSpeak.h"
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>

// Create AsyncWebServer object on port 8888
AsyncWebServer server(8888);

// Initialize Telegram BOT
#define BOTtoken "BOTtoken"  // your Bot Token (Get from Botfather)

// Use @myidbot to find out the chat ID of an individual or a group
// Also note that you need to click "start" on a bot before it can
// message you
#define CHAT_ID "-999999999"

WiFiClientSecure clientsecure;
UniversalTelegramBot bot(BOTtoken, clientsecure);

WiFiClient client;

// Uncomment one of the lines below for whatever DHT sensor type you're using!
#define DHTTYPE DHT11   // DHT 11

// DHT Sensor
uint8_t DHTPin = 33; 
               
// Initialize DHT sensor.
DHT dht(DHTPin, DHTTYPE);                

// Initialize Wifi connection to the router
char ssid[] = "network SSID"; //  your network SSID (name)
char password[] = "network password";    // your network password (use for WPA, or use as key for WEP)

int status = WL_IDLE_STATUS;

unsigned long myChannelNumber = 9999999;  // your Thingspeak Channel Number
const char * myWriteAPIKey = "WriteAPIKey";  // your Thingspeak WriteAPI Key

// Checks for new messages every 5 minutes.
int botRequestDelay = 300000;
unsigned long lastTimeBotRan;

void setup() {
  Serial.begin(115200);
  delay(100);

  // Initialize SPIFFS
  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  
  pinMode(DHTPin, INPUT);

  dht.begin();              

  // Attempt to connect to Wifi network
  Serial.print("Connecting to ");
  Serial.println(ssid);

  // Set WiFi to station mode and disconnect from an AP if it was Previously
  // connected
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("Connecting to WiFi..");
    delay(1000);
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  ThingSpeak.begin(client);
  
  bot.sendMessage(CHAT_ID, "Bot started up", "");
  
  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html");
  });

  // Start server
  server.begin();

}

// Handle what happens when you receive new messages
void handleNewMessages(int numNewMessages) {
  Serial.println("handleNewMessages");
  Serial.println(String(numNewMessages));

  for (int i=0; i<numNewMessages; i++) {
    // Chat id of the requester
    String chat_id = String(bot.messages[i].chat_id);
    if (chat_id != CHAT_ID){
      bot.sendMessage(chat_id, "Unauthorized user", "");
      continue;
    }
  }
}

void loop() {
  if (millis() > lastTimeBotRan + botRequestDelay) {
    float c = dht.readTemperature();
    float h = dht.readHumidity();

    ThingSpeak.setField(1,c);
    ThingSpeak.setField(2,h);
    ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey); 
  
    Serial.print("Humidity: ");
    Serial.print(h);
    Serial.println(" %\t");
    Serial.print("Temperature: ");
    Serial.print(c);
    Serial.println(" °C\t");

    String statusText = "";
      
    statusText = "Temperature: " + String(c) + " °C" +
                  "\nHumidity: " + String(h) + " %";

    bot.sendMessage(CHAT_ID, statusText, "");

    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while(numNewMessages) {
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }
}
