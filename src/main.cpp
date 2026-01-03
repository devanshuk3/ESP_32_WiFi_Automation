#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

#define LED_PIN 4                  
#define OLED_ADDR 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

struct WiFiCred {
  const char* ssid;
  const char* password;
};

WiFiCred wifiList[] = {
  { "jio_fiber", "nitin772799" },   
  { "Redmi",     "12345678" }       // Backup WiFi (mobile hotspot)
};

const int WIFI_COUNT = sizeof(wifiList) / sizeof(wifiList[0]);

// ================= FIREBASE =================
#define DATABASE_URL "https://esp-32-e4280-default-rtdb.firebaseio.com/"
#define DATABASE_SECRET "xn74imGcBTVZGvVocvBT8pL3ztJX7rskJy7WdEsa"

// ================= OBJECTS =================
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ================= STATE =================
bool ledState = false;

enum DisplayMode {
  INFO_MODE,
  STAR_MODE
};

DisplayMode currentMode = INFO_MODE;

// ================= WIFI CONNECT =================
bool connectToWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);

  for (int i = 0; i < WIFI_COUNT; i++) {
    Serial.print("\nTrying WiFi: ");
    Serial.println(wifiList[i].ssid);

    WiFi.begin(wifiList[i].ssid, wifiList[i].password);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
      delay(500);
      Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nWiFi connected");
      Serial.print("Connected to: ");
      Serial.println(wifiList[i].ssid);
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
      return true;
    }

    Serial.println("\nFailed, trying next network...");
    WiFi.disconnect(true);
    delay(1000);
  }

  Serial.println("\nNo WiFi network connected");
  return false;
}

// ================= OLED ================= 
void showInfoScreen() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextWrap(false);

  display.setTextSize(2);
  display.setCursor(0, 0);
  display.print("CONTROL");

  display.setCursor(0, 32);
  display.print("LED: ");
  display.print(ledState ? "ON" : "OFF");

  display.display();
}

void drawArcReactor(int cx, int cy, int phase) {
  display.fillCircle(cx, cy, 6, SSD1306_WHITE);
  display.drawCircle(cx, cy, 10, SSD1306_WHITE);
  display.drawCircle(cx, cy, 14, SSD1306_WHITE);

  for (int i = 0; i < 8; i++) {
    float angle = (i * 45 + phase) * DEG_TO_RAD;
    display.drawLine(
      cx + cos(angle) * 18,
      cy + sin(angle) * 18,
      cx + cos(angle) * 22,
      cy + sin(angle) * 22,
      SSD1306_WHITE
    );
  }
}

void arcReactorLoop() {
  static int phase = 0;
  phase = (phase + 6) % 360;

  int cx = SCREEN_WIDTH / 2;
  int cy = SCREEN_HEIGHT / 2;

  display.clearDisplay();
  drawArcReactor(cx, cy, phase);

  for (int i = 0; i < 10; i++) {
    int r = random(8, 26);
    float a = random(0, 360) * DEG_TO_RAD;
    display.drawPixel(cx + cos(a) * r, cy + sin(a) * r, SSD1306_WHITE);
  }

  display.display();
  delay(60);
}

// ================= FIREBASE READ =================
void readFirebase() {

  if (Firebase.RTDB.getBool(&fbdo, "/led")) {
    ledState = fbdo.boolData();
    digitalWrite(LED_PIN, ledState ? HIGH : LOW);
    Serial.print("Firebase /led = ");
    Serial.println(ledState);
  }

  if (Firebase.RTDB.getString(&fbdo, "/mode")) {
    String mode = fbdo.stringData();
    Serial.print("Firebase /mode = ");
    Serial.println(mode);

    if (mode == "star") {
      currentMode = STAR_MODE;
    } else {
      currentMode = INFO_MODE;
      showInfoScreen();
    }
  }
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Wire.begin(21, 22);
  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  randomSeed(millis());

  showInfoScreen();

  Serial.println("Connecting to WiFi...");
  if (!connectToWiFi()) {
    Serial.println("WiFi failed. Restarting ESP32...");
    delay(5000);
    ESP.restart();
  }

  config.database_url = DATABASE_URL;
  config.signer.tokens.legacy_token = DATABASE_SECRET;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  Serial.println("Firebase initialized");
}

// ================= LOOP =================
void loop() {

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi lost. Reconnecting...");
    connectToWiFi();
  }

  readFirebase();

  if (currentMode == STAR_MODE) {
    arcReactorLoop();
  }

  delay(300);
}
