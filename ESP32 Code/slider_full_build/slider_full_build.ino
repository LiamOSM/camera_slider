// VERSION 1.3
/*  Fully functional!
 *  Step 1. Connect to the wifi network called "Slider" with the password "123456789"
 *  Step 2. In a browser, go to 192.168.4.1
 *  Step 3. Press the "Calibrate" button (only necessary on the first startup)
 *  Step 4. Press the "Home" button
 *  A webpage with a large HTML slider will appear. Moving this slider will
 *  cause the motorized gantry to move in real-time. The arrow buttons on each
 *  end of the HTML slider allow for fine adjustment of its position.
 *  The start and end points can be set with the respective text fields, or by
 *  pressing "set to current". The speed can be set as a percentage (this appears
 *  by default) or as a time duration by first pressing "switch to time". Once the
 *  start point, end point, and time/speed are set, press the "run" button to make
 *  the slider perform the requested movement. 
 */

#include <WiFi.h>
#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>
#include <WebSocketsServer.h>
#include <EEPROM.h>

// Pin Definitions
#define enPin 19
#define dirPin 18
#define stepPin 5
#define rLim 21
#define lLim 23

// Constants
const char *ssid = "Slider";
const char *password =  "123456789";
const int dns_port = 53;
const int http_port = 80;
const int ws_port = 1024;
const int maxTravelSpeed = 1000;
const int minTravelSpeed = 25;
const int maxTime = 600;
const int minTime = 10;

// Global Variables
long sliderLength = 0;
long setpoint = 0;
long current = 0;
int travelSpeed = 25;
int calSpeed = 25;
unsigned long timeSpeed = 0; // in seconds
bool useTime = false;

AsyncWebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(ws_port);
char msg_buf[10];

String wsMessageStr;

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
}

void loop() {
  webSocket.loop();
}

// This function is called when a websocket request is
// received and the String "wsMessageStr" is updated
void handleWSMessage() {
  if (wsMessageStr.charAt(0) == 'm') {
    long newSetpoint = (sliderLength * wsMessageStr.substring(1).toInt()) / 100;
    Serial.print("Rapid To: ");
    Serial.println(newSetpoint);
    setpoint = newSetpoint;
    goTo(0);
  }
  else if (wsMessageStr.charAt(0) == 'r') {
    long newSetpoint = (sliderLength * wsMessageStr.substring(1).toInt()) / 100;
    Serial.print("Move To: ");
    Serial.println(newSetpoint);
    setpoint = newSetpoint;
    goTo(1);
  }
  else if (wsMessageStr.charAt(0) == 'c') {
    calibrate();
  }
  else if (wsMessageStr.charAt(0) == 'h') {
    homePosition();
  }
  else if (wsMessageStr.charAt(0) == 's') {
    // map the speed as a percentage to the travel speed delay value (microsecs)
    travelSpeed = map(wsMessageStr.substring(1).toInt(), 0, 100, maxTravelSpeed, minTravelSpeed);
    Serial.print("travelSpeed = ");
    Serial.println(travelSpeed);
    useTime = false;
  }
  else if (wsMessageStr.charAt(0) == 't') {
    // time is given instead of a speed
    // speed must be calculated when a run command is received
    timeSpeed = wsMessageStr.substring(1).toInt(); // seconds
    timeSpeed = constrain(timeSpeed, minTime, maxTime); // limit to be between 1 min and 5 mins
    Serial.print("timeSpeed = ");
    Serial.println(timeSpeed);
    useTime = true;
  }
}

void goTo(int travelType) {
  int dir;
  long tempSpeed;

  if (setpoint > sliderLength)
    setpoint = sliderLength;

  long delta = current - setpoint;
  delta = abs(delta); // <- don't combine this with the line above, the abs function is whack
  if (delta == 0)
    return;

  if (travelType == 0) { // rapid travel speed
    tempSpeed = calSpeed;
  }
  else { // precision travel speed
    if (useTime) {
      // total time in ms is delta*2*tempSpeed
      // therefore tempSpeed = 1000*1000*timeSpeed/delta/2
      tempSpeed = 500000 * timeSpeed / delta;
    }
    else {
      tempSpeed = travelSpeed;
    }
  }
  if (tempSpeed >= 500000)
    return;

  Serial.print("tempSpeed = ");
  Serial.println(tempSpeed);

  if (setpoint > current)
    dir = 0;
  else
    dir = 1;
  digitalWrite(dirPin, dir);

  while (delta > 0) {
    if (dir && digitalRead(rLim) == LOW) {
      current = 0;
      break;
    }
    else if (!dir && digitalRead(lLim) == LOW) {
      current = sliderLength;
      break;
    }
    delta = current - setpoint;
    delta = abs(delta);
    //    Serial.print("Current: ");
    //    Serial.print(current);
    //    Serial.print(", Setpoint = ");
    //    Serial.println(setpoint);
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(tempSpeed);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(tempSpeed);
    if (current < setpoint)
      current++;
    else
      current--;
  }

}

// call this function on startup and if the client requests a move to position zero
void homePosition() {
  digitalWrite(enPin, LOW);

  // Move right until limit switch is reached
  digitalWrite(dirPin, HIGH);
  while (digitalRead(rLim) != LOW) {
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

  current = 0; // we know the position is now zero

}
