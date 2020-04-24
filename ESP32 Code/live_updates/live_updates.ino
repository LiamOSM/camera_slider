#include "WiFi.h"
#include "ESPAsyncWebServer.h"

const char* ssid = "ESP32";
const char* password = "password";

AsyncWebServer server(80);

String getValue1() {
  return String(millis()/1000);  
}

String getValue2() {
  return String(random(999));  
}

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<body>
  <h2>liams first web site</h2>
  <p>
    Value #1:
    <span id="value1">%value1%</span>
  </p>
  <p>
    Value #2:
    <span id="value2">%value2%</span>
  </p>
</body>

<script>
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("value1").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/value1", true);
  xhttp.send();
}, 1000 ) ;

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("value2").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/value2", true);
  xhttp.send();
}, 1000 ) ;
</script>
</html>)rawliteral";

String processor(const String& var){
  //Serial.println(var);
  if(var == "value1"){
    return getValue1();
  }
  else if(var == "value2"){
    return getValue2();
  }
  return String();
}

void setup(){
  Serial.begin(9600);

  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("\nAccess Point IP address: ");
  Serial.println(IP);
  server.begin();

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });
  server.on("/value1", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", getValue1().c_str());
  });
  server.on("/value2", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", getValue2().c_str());
  });

  server.begin();
}
 
void loop(){
  
}