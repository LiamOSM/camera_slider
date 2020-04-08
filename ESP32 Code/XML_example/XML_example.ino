#include <WiFiClient.h>
#include <ESP32WebServer.h>
#include <WiFi.h>
#include "SPIFFS.h"

const char* ssid = "Bill Wi The Science Fi";
const char* password = "summer7645";

ESP32WebServer server(80);

void handleRoot()
{
  File myFile = SPIFFS.open("/index.html");
  if (myFile) 
  {  
    size_t sent = server.streamFile(myFile, "text/html");
    myFile.close();
    Serial.println("Request handled");
  } 
  else
  {
    Serial.println("Error opening file");
  }
}

void handleMotors() 
{ 
  String motorState = "OFF";
  String t_state = server.arg("motorState");

  Serial.print("D\r\n"); //Disable motors
  delay(50);

  if(t_state.startsWith("U"))  //Drive Forward (UP Arrow)
  {
    Serial.print("M0F70\r\n");
    delay(50);
    Serial.print("M1F70\r\n");
    delay(50);
    Serial.print("E\r\n");
  }
  else if(t_state.startsWith("D")) //Reverse (DOWN Arrow)
  {
    Serial.print("M0R70\r\n");
    delay(50);
    Serial.print("M1R70\r\n");
    delay(50);
    Serial.print("E\r\n");
  }
  else if(t_state.startsWith("R")) //Turn Right (Right Arrow)
  {
    Serial.print("M0F50\r\n");
    delay(50);
    Serial.print("M1R50\r\n");
    delay(50);
    Serial.print("E\r\n");
  }
  else if(t_state.startsWith("L")) //Turn Left (LEFT Arrow)
  {
    Serial.print("M0R50\r\n");
    delay(50);
    Serial.print("M1F50\r\n");
    delay(50);
    Serial.print("E\r\n");
  }

   server.send(200, "text/plain", motorState); //Send web page
}

// cannot handle request so return 404
void handleNotFound()
{
  server.send(404, "text/plain", "File not found");
}

void setup(){
  Serial.begin(9600);
  WiFi.begin(ssid, password);

  Serial.println();

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/setMotors", handleMotors);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("Server started");
  if(!SPIFFS.begin(true)){
    Serial.println("Error mounting SPIFFS");
    while(1){}
  }
}

void loop(){
  server.handleClient();
}
