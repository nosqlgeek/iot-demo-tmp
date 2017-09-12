//
// IoT Redis Workshop
// F.Cerbelle
//
// Goal : Understand and use the RESP Protocol and PubSub 
//        for realtime sensor measure from a microcontroller
//        to a web dashboard
//
// Base file :
// - Serial console initialized with DEBUG, STATS and PROFILING
// - Disabled WiFi debug on Serial
// - Wifi network configuration and connection
// - LED configuration and blink on stats
// - Wifi connection sample with send and receive
// - Sensor read every 5 seconds
//
// Notice that everything is done to be non-blocking, 
// the main loop has to run as fast as possible
//

// Configuration
#define DEBUG
#define PROF
#define STATS
//#define STATS_MEM
#define STATS_HOOK digitalWrite(LED_BUILTIN,((digitalRead(LED_BUILTIN)==HIGH)?LOW:HIGH))
#define STATS_PERIOD 1000

//  your network SSID (name)
#define WIFI_SSID "RedisLabsGuest"
#define WIFI_PASS "redisredis"

#define REDISHOST "redis-14658.demo.francois.demo-rlec.redislabs.com"
#define REDISPORT 14658

#include <ESP8266WiFi.h>
#include "RedisCommand.h"
#include "tools.h"


WiFiClient redisConnection;
IPAddress redisIP;

/********/
/* Main */
/********/
void setup() {
  Serial.begin(115200);
//  Serial.setDebugOutput(true);
  while (!Serial);
  DEBUG_PRINTLN("Serial initialized.");

  // WIFI stuff
  DEBUG_PRINT("Connecting to ");
  DEBUG_PRINT(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    DEBUG_PRINT(".");
  }
  DEBUG_PRINTLN("");
  DEBUG_PRINT("WiFi (");
  DEBUG_PRINT(WiFi.macAddress());
  DEBUG_PRINT(") connected with IP ");
  DEBUG_PRINTLN(WiFi.localIP());
  DEBUG_PRINT("DNS0 : ");
  DEBUG_PRINTLN(WiFi.dnsIP(0));
  DEBUG_PRINT("DNS1 : ");
  DEBUG_PRINTLN(WiFi.dnsIP(1));

  pinMode(LED_BUILTIN, OUTPUT);
  for (int i = 0; i<5; i++) {
    digitalWrite(LED_BUILTIN,HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN,LOW);
    delay(200);
  }
}

unsigned long lastSensorRead=0;

RedisCommand_t cmd;
char* szRESP;

void loop() {
  STATS_LOOP

   if (!redisConnection.connected()) {
      DEBUG_PRINT("Opening connection to ");
      DEBUG_PRINT(REDISHOST);
      DEBUG_PRINT("(");
  
      WiFi.hostByName(REDISHOST, redisIP);
      DEBUG_PRINT(redisIP);
      DEBUG_PRINT("):");
      DEBUG_PRINT(REDISPORT);
      DEBUG_PRINT("...");
      
      if (!redisConnection.connect(redisIP, REDISPORT)) {
        DEBUG_PRINTLN(" Failed");
      } else {
        DEBUG_PRINTLN(" Succeed");
      }

      // Send hardcoded AUTH in RESP
      redisConnection.write("*2\r\n$4\r\nAUTH\r\n$3\r\niot\r\n" );
   }

   rediscommand_init(cmd);
   rediscommand_add(cmd, "LPUSH");
   rediscommand_add(cmd, (String("v:")+WiFi.macAddress()).c_str());
   rediscommand_add(cmd, analogRead(0));
   szRESP=rediscommand_tochar(cmd);
   redisConnection.print(szRESP);
   free(szRESP);

   rediscommand_init(cmd);
   rediscommand_add(cmd, "PUBLISH");
   rediscommand_add(cmd,"refreshvalues");
   rediscommand_add(cmd, WiFi.macAddress().c_str());
   szRESP=rediscommand_tochar(cmd);
   redisConnection.print(szRESP);
   free(szRESP);
   

   // Expect a reply and busy (bad) wait for it
   while(redisConnection.available()==0);
   // Output to the console all the received bytes as chars
   while(redisConnection.available()!=0)
    Serial.print((char)redisConnection.read());

  /*
  if ((millis() - lastSensorRead)>5000) {
    PROF_START(SensorRead);
    Serial.print("Sensor value (0-1024) : ");
    Serial.println(analogRead(0));
    PROF_STOP(SensorRead);
    lastSensorRead = millis();
  }
  */
  
}
