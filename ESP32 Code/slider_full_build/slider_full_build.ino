#include <WiFi.h>
#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>
#include <WebSocketsServer.h>
#include <EEPROM.h>

// Compile-time assertions
#define enPin 19
#define dirPin 18
#define stepPin 5
#define rLim 22
#define lLim 23

// Constants
const char *ssid = "ESP32";
const char *password =  "123456789";
const int dns_port = 53;
const int http_port = 80;
const int ws_port = 1024;

// Global Variables
long sliderLength = 0;
long setpoint = 0;
long current = 0;
int speed = 50;
int calSpeed = 30;
bool kill = true;


AsyncWebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(ws_port);
char msg_buf[10];

String wsMessageStr;

// Interrupt service routines
void IRAM_ATTR rLimISR() { // zero position (home)
  current = 0;
  // kill = true;
  //Serial.println(current);
}
void IRAM_ATTR lLimISR() { // positive limit
  current = sliderLength;
  // kill = true;
  // Serial.println(current);
}

// Callback: receiving any WebSocket message
void onWebSocketEvent(uint8_t client_num, WStype_t type, uint8_t * payload, size_t length) {
  // Figure out the type of WebSocket event
  switch (type) {

    // Client has disconnected
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", client_num);
      break;

    // New client has connected
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(client_num);
        Serial.printf("[%u] Connection from ", client_num);
        Serial.println(ip.toString());
      }
      break;

    // Handle text messages from client
    case WStype_TEXT:
      // The message, as a String object (easier to work with in my opinion)
      wsMessageStr = (char *)payload;
      // Print out raw message
      Serial.printf("[%u] Received text: ", client_num);
      Serial.println(wsMessageStr);
      // handle the request
      handleWSMessage();
      break;

    // For everything else: do nothing
    case WStype_BIN:
    case WStype_ERROR:
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
    default:
      break;
  }
}

void onIndexRequest(AsyncWebServerRequest *request) {
  // Callback: send homepage
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                 "] HTTP GET request of " + request->url());
  request->send(SPIFFS, "/page.html", "text/html");
}

void onFaviconRequest(AsyncWebServerRequest *request) {
  // Callback: send favicon
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                 "] HTTP GET request of " + request->url());
  request->send(SPIFFS, "/favicon.ico", "image/ico");
}

void onCSSRequest(AsyncWebServerRequest *request) {
  // Callback: send style sheet
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                 "] HTTP GET request of " + request->url());
  request->send(SPIFFS, "/style.css", "text/css");
}

void onPageNotFound(AsyncWebServerRequest *request) {
  // Callback: send 404 if requested file does not exist
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                 "] HTTP GET request of " + request->url());
  request->send(404, "text/plain", "Error (404)\nYou've made a huge mistake!");
}

void setup() {
  // Start Serial port
  Serial.begin(115200);

  EEPROM.begin(512);
  EEPROM.get(0, sliderLength);
  Serial.print("Slider length from EEPROM: ");
  Serial.print(sliderLength);
  Serial.println(" steps");

  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);
  pinMode(enPin, OUTPUT);
  pinMode(rLim, INPUT_PULLUP);
  pinMode(lLim, INPUT_PULLUP);
  digitalWrite(dirPin, LOW);

  // Make sure we can read the file system
  if ( !SPIFFS.begin()) {
    Serial.println("Error mounting SPIFFS");
    while (1);
  }

  // Start access point
  WiFi.softAP(ssid, password);

  // Print our IP address
  Serial.println();
  Serial.print("My IP address: ");
  Serial.println(WiFi.softAPIP());
  Serial.println();

  // On HTTP request for root, provide index.html file
  server.on("/", HTTP_GET, onIndexRequest);

  // On HTTP request for style sheet, provide style.css
  server.on("/style.css", HTTP_GET, onCSSRequest);

  // On HTTP request for favicon, provide the icon
  server.on("/favicon.ico", HTTP_GET, onFaviconRequest);

  // Handle requests for pages that do not exist
  server.onNotFound(onPageNotFound);

  // Start web server
  server.begin();

  // Start WebSocket server and assign callback
  webSocket.begin();
  webSocket.onEvent(onWebSocketEvent);

  // No need to calibrate on every power cycle since length comes from EEPROM
  // calibrate();

  // enable interrupts for limit switches
  attachInterrupt(rLim, rLimISR, FALLING);
  attachInterrupt(lLim, lLimISR, FALLING);
}

void loop() {
  webSocket.loop();
}

// This function is called when a websocket request is
// received and the String "wsMessageStr" is updated
void handleWSMessage() {
  if (wsMessageStr.charAt(0) == 'm') {
    long newSetpoint = 100 * wsMessageStr.substring(1).toInt();
    Serial.print("Go To: ");
    Serial.println(newSetpoint);
    setpoint = newSetpoint;
    goTo();
  }
  else if (wsMessageStr.charAt(0) == 'c') {
    calibrate();
  }
  else if (wsMessageStr.charAt(0) == 'h') {
    homePosition();
  }
}

void goTo() {
  long delta = current - setpoint;
  delta = abs(delta);
  if (setpoint > current)
    digitalWrite(dirPin, LOW);
  else
    digitalWrite(dirPin, HIGH);
  for (long i = 0; i < delta; i++) {
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(calSpeed);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(calSpeed);
  }
  current = setpoint;
}

// call this function on startup and if the client requests a move to position zero
void homePosition() {
  detachInterrupt(rLim);
  detachInterrupt(lLim);
  digitalWrite(enPin, LOW);

  // Move right until limit switch is reached
  digitalWrite(dirPin, HIGH);
  while (digitalRead(rLim) != 0) {
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(calSpeed);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(calSpeed);
  }
  current = 0;
  setpoint = 0;
}

void calibrate() {
  Serial.print("Calibrating...");
  detachInterrupt(rLim);
  detachInterrupt(lLim);
  digitalWrite(enPin, LOW);
  unsigned long temp = 0;

  // Move left (out)
  digitalWrite(dirPin, LOW);
  while (digitalRead(lLim) != 0) {
    // move left until the limit is reached
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(calSpeed);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(calSpeed);
  }
  delay(250);

  // Move right (home)
  digitalWrite(dirPin, HIGH);
  while (digitalRead(rLim) != 0) {
    // move right until the limit is reached, while counting steps
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(calSpeed);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(calSpeed);
    temp++;
  }
  delay(250);
  sliderLength = temp;
  Serial.println("Done.");

  EEPROM.put(0, sliderLength);
  EEPROM.commit();
  Serial.println("Length put in EEPROM");

  Serial.print("Slider length: ");
  Serial.println(temp);
}
