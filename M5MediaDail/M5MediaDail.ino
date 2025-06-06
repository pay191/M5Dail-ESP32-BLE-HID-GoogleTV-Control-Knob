#include <M5Dial.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <BLEHIDDevice.h>
#include "HIDKeyboardTypes.h"

BLEHIDDevice* hid;
BLECharacteristic* input;
BLECharacteristic* output;
bool isConnected = false;

const char* ssid = "SKYHAUS-G";
const char* password = "FULLSEND";

struct LocationInfo {
  String city;
  String region;
  String country;
  String timezone;
  bool success;
};

// Add this mapping near the top of your file
std::map<String, String> timezonePOSIXMap = {
  {"America/Los_Angeles", "PST8PDT,M3.2.0,M11.1.0"},
  {"America/New_York", "EST5EDT,M3.2.0,M11.1.0"},
  {"America/Chicago", "CST6CDT,M3.2.0,M11.1.0"},
  {"America/Denver", "MST7MDT,M3.2.0,M11.1.0"},
  {"America/Phoenix", "MST7"},
  {"UTC", "UTC0"},
  // Add more mappings as needed
};


String currentTimezone = "UTC";
unsigned long lastClockUpdate = 0;
String lastDisplayedTime = "";

long currentEncoder = 0;
long lastEncoder = 0;
int encoderStepstoChange = 4; //On the M5 Dail it seems like there are 4 encoder changes per physical detent.
int volumePressesPerChange = 5;


void showBootScreen() {
  M5Dial.Display.setTextColor(WHITE);
  M5Dial.Display.fillScreen(BLACK);
  M5Dial.Display.setTextFont(&fonts::Font2);
  M5Dial.Display.setTextDatum(middle_center);
  M5Dial.Display.drawString("MAGIC", M5Dial.Display.width() / 2, M5Dial.Display.height() / 2 - 20);
  M5Dial.Display.drawString("DIAL", M5Dial.Display.width() / 2, M5Dial.Display.height() / 2 + 20);
  delay(3000);
}

void DisplayConnectionText() {
  int bannerHeight = M5Dial.Display.height() / 6;
  uint32_t bgColor = isConnected ? 0x007BFF : 0x999999;
  const char* label = isConnected ? "PAIRED" : "DISCONNECTED";

  M5Dial.Display.fillRect(0, 0, M5Dial.Display.width(), bannerHeight, bgColor);

  M5Dial.Display.setTextColor(WHITE);
  M5Dial.Display.setTextDatum(middle_center);
  M5Dial.Display.setTextSize(1);
  M5Dial.Display.setTextFont(&fonts::Font2);
  M5Dial.Display.drawString(label, M5Dial.Display.width() / 2, bannerHeight / 2);

  M5Dial.Display.drawFastHLine(0, bannerHeight, M5Dial.Display.width(), WHITE);
}

void DisplayStatusText(const char* text) {
  M5Dial.Display.fillScreen(BLACK);
  M5Dial.Display.setTextFont(&fonts::Font2);
  M5Dial.Display.setTextColor(WHITE);
  M5Dial.Display.setTextDatum(middle_center);
  M5Dial.Display.drawString(text, M5Dial.Display.width() / 2, M5Dial.Display.height() / 2);
}

void DrawClock() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return;

  char buf[6];
  strftime(buf, sizeof(buf), "%H:%M", &timeinfo);

  String timeStr = String(buf);
  if (timeStr == lastDisplayedTime) return;
  lastDisplayedTime = timeStr;

  int clockHeight = M5Dial.Display.height() / 4;
  int y = M5Dial.Display.height() - clockHeight;

  M5Dial.Display.fillRect(0, y, M5Dial.Display.width(), clockHeight, BLACK);
  M5Dial.Display.setTextFont(&fonts::Font2);
  M5Dial.Display.setTextSize(2);
  M5Dial.Display.setTextColor(WHITE);
  M5Dial.Display.setTextDatum(middle_center);
  M5Dial.Display.drawString(timeStr.c_str(), M5Dial.Display.width() / 2, y + clockHeight / 2);
}

LocationInfo getLocationFromIP() {
  LocationInfo info;
  info.success = false;

  HTTPClient http;
  http.begin("http://ip-api.com/json/");
  int httpCode = http.GET();

  if (httpCode == 200) {
    String payload = http.getString();
    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
      info.city = doc["city"].as<String>();
      info.region = doc["regionName"].as<String>();
      info.country = doc["country"].as<String>();
      info.timezone = doc["timezone"].as<String>();
      info.success = true;
    }
  }
  http.end();
  return info;
}

void connectToWiFi() {
  WiFi.begin(ssid, password);
  int tries = 0;

  DisplayStatusText("Connecting WiFi...");

  while (WiFi.status() != WL_CONNECTED && tries < 20) {
    delay(500);
    Serial.print(".");
    tries++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    String ip = WiFi.localIP().toString();
    Serial.println("WiFi connected: " + ip);
    DisplayStatusText(("IP: " + ip).c_str());
    delay(1000);

    LocationInfo loc = getLocationFromIP();
    if (loc.success) {

      currentTimezone = loc.timezone;

      // Use POSIX mapping if available
      if (timezonePOSIXMap.find(currentTimezone) != timezonePOSIXMap.end()) {
        currentTimezone = timezonePOSIXMap[currentTimezone];
      } else {
        currentTimezone = "UTC0"; // fallback
      }

      configTzTime(currentTimezone.c_str(), "pool.ntp.org", "time.nist.gov", "time.google.com");
      delay(1000);  // give time for timezone and NTP to sync

      time_t now = time(nullptr);
      struct tm timeinfo;
      localtime_r(&now, &timeinfo);  // force timezone application

      Serial.println("Time sync successful.");
      Serial.printf("Time (adjusted): %02d:%02d\n", timeinfo.tm_hour, timeinfo.tm_min);

      int wait = 0;
      while (!getLocalTime(&timeinfo) && wait < 10) {
        delay(500);
        wait++;
      }

      if (wait < 10) {
        Serial.println("Time sync successful.");
      } else {
        Serial.println("Time sync failed.");
      }

      Serial.printf("Location: %s, %s (%s)\nTimezone: %s\n",
                    loc.city.c_str(), loc.region.c_str(), loc.country.c_str(), loc.timezone.c_str());

      String msg = "City: " + loc.city;
      DisplayStatusText(msg.c_str());
      delay(1000);
    } else {
      DisplayStatusText("Loc Lookup Failed");
      delay(1000);
    }

    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
  } else {
    Serial.println("WiFi failed.");
    DisplayStatusText("WiFi Failed");
    delay(2000);
  }
}

class MyCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    BLE2902* desc = (BLE2902*)input->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
    desc->setNotifications(true);
    isConnected = true;
    DisplayConnectionText();
  }
  void onDisconnect(BLEServer* pServer) {
    isConnected = false;
    DisplayConnectionText();
  }
};

void sendKey(const uint8_t usage, const uint8_t modifier = 0) {
  uint8_t keyDown[] = {modifier, 0x00, usage, 0x00, 0x00, 0x00, 0x00, 0x00};
  input->setValue(keyDown, sizeof(keyDown));
  input->notify();
  delay(100);
  uint8_t keyUp[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  input->setValue(keyUp, sizeof(keyUp));
  input->notify();

}

void StartBLEServer() {
  BLEDevice::init("M5Dial HID");
  BLEServer* pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyCallbacks());

  hid = new BLEHIDDevice(pServer);
  input = hid->inputReport(1);
  output = hid->outputReport(1);

  hid->manufacturer()->setValue("HillValleyLabs");
  hid->pnp(0x02, 0xe502, 0xa111, 0x0210);
  hid->hidInfo(0x00, 0x01);

  const uint8_t reportMap[] = {
    USAGE_PAGE(1), 0x01,
    USAGE(1), 0x06,
    COLLECTION(1), 0x01,
    REPORT_ID(1), 0x01,
    USAGE_PAGE(1), 0x07,
    USAGE_MINIMUM(1), 0xE0,
    USAGE_MAXIMUM(1), 0xE7,
    LOGICAL_MINIMUM(1), 0x00,
    LOGICAL_MAXIMUM(1), 0x01,
    REPORT_SIZE(1), 0x01,
    REPORT_COUNT(1), 0x08,
    HIDINPUT(1), 0x02,
    REPORT_COUNT(1), 0x01,
    REPORT_SIZE(1), 0x08,
    HIDINPUT(1), 0x01,
    REPORT_COUNT(1), 0x05,
    REPORT_SIZE(1), 0x01,
    USAGE_PAGE(1), 0x08,
    USAGE_MINIMUM(1), 0x01,
    USAGE_MAXIMUM(1), 0x05,
    HIDOUTPUT(1), 0x02,
    REPORT_COUNT(1), 0x01,
    REPORT_SIZE(1), 0x03,
    HIDOUTPUT(1), 0x01,
    REPORT_COUNT(1), 0x06,
    REPORT_SIZE(1), 0x08,
    LOGICAL_MINIMUM(1), 0x00,
    LOGICAL_MAXIMUM(1), 0x65,
    USAGE_PAGE(1), 0x07,
    USAGE_MINIMUM(1), 0x00,
    USAGE_MAXIMUM(1), 0x65,
    HIDINPUT(1), 0x00,
    END_COLLECTION(0)
  };

  hid->reportMap((uint8_t*)reportMap, sizeof(reportMap));
  hid->startServices();

  BLESecurity* pSecurity = new BLESecurity();
  pSecurity->setAuthenticationMode(ESP_LE_AUTH_BOND);

  BLEAdvertising* pAdvertising = pServer->getAdvertising();
  pAdvertising->setAppearance(HID_KEYBOARD);
  pAdvertising->addServiceUUID(hid->hidService()->getUUID());
  pAdvertising->start();
}

void setup() {
  M5Dial.begin();
  Serial.begin(115200);
  M5Dial.Display.setBrightness(200);

  M5Dial.Encoder.begin();  // ✅ Prevents null reads

  showBootScreen();
  DisplayStatusText("Initializing...");
  connectToWiFi();
  delay(10000);
  StartBLEServer();
  DisplayStatusText("Initialized.");
  DisplayConnectionText();
}

void loop() {
  M5.update();

  if (isConnected && M5Dial.BtnA.wasPressed()) {
    
    sendKey(0x43); // KEY_F10 according to HID Usage Table

    //const char* hello = "Hello\n";
    //hile (*hello) {
    //  sendKey(keymap[(uint8_t)*hello].usage, keymap[(uint8_t)*hello].modifier);
    //  hello++;
    //}
  }

  if (isConnected && M5Dial.BtnA.pressedFor(1500)) {
    sendKey(0x43); // KEY_F10 according to HID Usage Table
  }


currentEncoder = M5Dial.Encoder.read();
long delta = currentEncoder - lastEncoder;

if(delta !=0){
  Serial.printf("Raw: %d, Delta: %d\n", currentEncoder, delta);
}

// Only act if we’ve moved 2 or more notches
if (abs(delta) >= encoderStepstoChange) {
  
  int steps = delta / encoderStepstoChange;  // ±1, ±2, etc.
  int volumeClicks = abs(steps) * volumePressesPerChange;

  Serial.printf("Volume %s x %d\n", (steps > 0 ? "UP" : "DOWN"), volumeClicks);

  for (int i = 0; i < volumeClicks; i++) {
    sendKey(steps > 0 ? 0x45 : 0x44);  // F12 = Volume Up, F11 = Volume Down
    delay(20);
  }

}


  if (millis() - lastClockUpdate > 30000) {
    lastClockUpdate = millis();
    DrawClock();
  }

  delay(100);

}
