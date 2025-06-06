#include <M5Dial.h>
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

void DisplayConnectionText() {
  M5Dial.Display.setCursor(10, 10);
  M5Dial.Display.fillRect(0, 0, 240, 30, BLACK);
  M5Dial.Display.print(isConnected ? "Connected" : "Not connected");
}

void DisplayStatusText(const char* text) {
  M5Dial.Display.setCursor(10, 40);
  M5Dial.Display.fillRect(0, 30, 240, 40, BLACK);
  M5Dial.Display.print(text);
}

void DisplayGuide() {
  M5Dial.Display.setCursor(10, 90);
  M5Dial.Display.println("Tap Btn: Hello");
  M5Dial.Display.println("Hold Btn: Ctrl+Alt+Del");
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

  uint8_t keyUp[] = {0x00};
  input->setValue(keyUp, sizeof(keyDown));
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
  M5Dial.Display.fillScreen(BLACK);
  M5Dial.Display.setTextColor(WHITE);
  M5Dial.Display.setTextSize(2);

  DisplayStatusText("Initializing...");
  DisplayConnectionText();
  DisplayGuide();
  StartBLEServer();
  DisplayStatusText("Initialized.");
}

void loop() {
  M5.update();

  if (isConnected && M5Dial.BtnA.wasPressed()) {
    const char* hello = "Hello\n";
    while (*hello) {
      sendKey(keymap[(uint8_t)*hello].usage, keymap[(uint8_t)*hello].modifier);
      hello++;
    }
  }

  if (isConnected && M5Dial.BtnA.pressedFor(1500)) {
    sendKey(0x4C, KEY_CTRL | KEY_ALT); // CTRL+ALT+DEL
  }
} 
