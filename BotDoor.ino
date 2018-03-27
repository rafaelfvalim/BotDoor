
#include <Wire.h>
#include "SSD1306Wire.h"
#include "OLEDDisplayUi.h"
#include "images.h"
#include <PubSubClient.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <WiFiManager.h>
#include <WiFiClient.h>
WebServer server(80);

SSD1306Wire  display(0x3c, 5, 4);
OLEDDisplayUi ui     ( &display );

const char *ssid         = "VivoFibra1222";
const char *password     = "lapiseira";
WiFiClient espClient;

const char* mqtt_server = "m11.cloudmqtt.com";
const char* mqtt_user = "pgusyofr";
const char* mqtt_id = "pgusyofr";
const char* mqtt_password = "nvuP1aQKdhlH";
const char* mqtt_pub_topic = "cmd_botdoor";
const char* mqtt_pub_subsc = "id_botdoor";
enum COMANDO { ABRIR, FECHAR, NAODEFINIDO };
int ledPin = 0;

PubSubClient client(espClient);
String dados;
String mac = WiFi.macAddress();

void showIP(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_16);
  display->drawString(0 + x, 10 + y, WiFi.localIP().toString());
}

void msOverlay(OLEDDisplay *display, OLEDDisplayUiState* state) {
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->setFont(ArialMT_Plain_10);
  display->drawString(128, 0, String(millis()));
}

void drawFrame1(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->drawXbm(x + 34, y + 14, WiFi_Logo_width, WiFi_Logo_height, WiFi_Logo_bits);
}

void drawFrame2(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_10);
  display->drawString(0 + x, 10 + y, "Arial 10");

  display->setFont(ArialMT_Plain_16);
  display->drawString(0 + x, 20 + y, "Arial 16");

  display->setFont(ArialMT_Plain_24);
  display->drawString(0 + x, 34 + y, "Arial 24");
}

void drawFrame3(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  // Text alignment demo
  display->setFont(ArialMT_Plain_10);

  // The coordinates define the left starting point of the text
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(0 + x, 11 + y, "Left aligned (0,10)");

  // The coordinates define the center of the text
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->drawString(64 + x, 22 + y, "Center aligned (64,22)");

  // The coordinates define the right end of the text
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->drawString(128 + x, 33 + y, "Right aligned (128,33)");
}

void drawFrame4(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  // Demo for drawStringMaxWidth:
  // with the third parameter you can define the width after which words will be wrapped.
  // Currently only spaces and "-" are allowed for wrapping
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_10);
  display->drawStringMaxWidth(0 + x, 10 + y, 128, "Lorem ipsum\n dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore.");
}

void drawFrame5(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setFont(ArialMT_Plain_10);
  // The coordinates define the left starting point of the text
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(0 + x, 11 + y, "Recebido[ " + dados + " ] ");
}

FrameCallback frames[] = { showIP, drawFrame1, drawFrame2, drawFrame3, drawFrame4, drawFrame5 };
int frameCount = 6;
OverlayCallback overlays[] = { msOverlay };
int overlaysCount = 1;

void barraProgresso(int valor) {
  display.println("Connecting...");
  display.drawProgressBar(10, 32, 100, 10, valor);
  delay(2000);
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  dados = "";
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    dados.concat((char)payload[i]);
  }
  COMANDO cmd = getComando(dados);
  if (cmd != COMANDO::NAODEFINIDO) {
    executaComando();
  }

  ui.switchToFrame(5);
  Serial.println();
}
char* string2char(String command) {
  if (command.length() != 0) {
    char *p = const_cast<char*>(command.c_str());
    return p;
  }
}

COMANDO getComando(String comando) {
  if (comando == "ABRIR") {
    return COMANDO::ABRIR;
  }
  if (comando == "FECHAR") {
    return COMANDO::FECHAR;
  }
  return COMANDO::NAODEFINIDO;
}


void mqttConnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(mqtt_user , mqtt_id, mqtt_password)) {
      Serial.println("connected");
      char* pub = string2char(mac);
      client.publish(mqtt_pub_subsc, pub);
      client.subscribe(mqtt_pub_topic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}
void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  display.init();
  display.flipScreenVertically();
  display.setContrast(255);
  display.clear();
  display.println("Connecting...");
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
  display.println(myWiFiManager->getConfigPortalSSID());
}


void wifiConect() {

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  //reset saved settings
  wifiManager.resetSettings();
  //set custom ip for portal
  //wifiManager.setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));
  wifiManager.setAPCallback(configModeCallback);
  if (!wifiManager.autoConnect()) {
    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    wifiManager.resetSettings();
    delay(1000);
  }
  //fetches ssid and pass from eeprom and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  wifiManager.autoConnect("AutoConnectAP");
  //or use this for auto generated name ESP + ChipID
  //wifiManager.autoConnect();

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");
  display.println("Conn");

}

void setup() {
  Serial.begin(9600);
  pinMode (ledPin, OUTPUT);
  executaComando();
  display.init();
  display.flipScreenVertically();
  display.setContrast(255);
  display.clear();
  wifiConect();
  // WiFi.begin(ssid, password);
  //  int count = 0;
  //  while (WiFi.status() != WL_CONNECTED) {
  //    barraProgresso(count);
  //    delay ( 100 );
  //    count++;
  //  }
  //  barraProgresso(100);
  //  delay(2000);
  ui.setTargetFPS(60);
  ui.setActiveSymbol(activeSymbol);
  ui.setInactiveSymbol(inactiveSymbol);
  ui.setIndicatorPosition(BOTTOM);
  ui.setIndicatorDirection(LEFT_RIGHT);
  ui.setFrameAnimation(SLIDE_LEFT);
  ui.setFrames(frames, frameCount);
  ui.setOverlays(overlays, overlaysCount);
  ui.init();
  display.flipScreenVertically();
  client.setServer(mqtt_server, 14986);
  client.setCallback(callback);
  mqttConnect();
}

void executaComando() {
  digitalWrite(ledPin, !digitalRead(ledPin));
}

void loop() {
  int remainingTimeBudget = ui.update();
  if (remainingTimeBudget > 0) {
    delay(remainingTimeBudget);
  }
  if (!client.connected()) {
    mqttConnect();
  }
  client.loop();
}
