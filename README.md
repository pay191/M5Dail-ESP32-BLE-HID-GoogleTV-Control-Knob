# ESP32 BLE HID Keyboard for Google TV Automation

Use an ESP32 to control Google TV via Bluetooth HID keyboard emulation. This project lets you send key commands wirelessly to your Google TV device — great for volume control, launching Google Home camera feeds, and automating common actions.

## 🎯 Features

- 🔊 Control volume (up, down, mute)
- 📷 Trigger Google Home camera feeds
- 🎮 Emulate a Bluetooth keyboard over BLE
- 🧠 Automate key sequences for Google TV navigation
- 🔧 Works with custom hardware (e.g., rotary encoders, buttons)

## 📦 Hardware Used

- ESP32 (any BLE-capable dev board)
- Optional: rotary encoder (e.g., M5Stack Dial), push buttons, or touchscreen
- Power supply or battery pack

## 🧪 Demo Use Cases

- Twist knob to increase/decrease volume
- Tap to mute/unmute
- Long press to launch Google Home security camera
- Map custom keys to automate sequences like navigating menus

## 🚀 Getting Started

1. **Install Arduino IDE** and ESP32 board definitions  
2. **Clone this repo**  
   ```bash
   git clone https://github.com/your-username/esp32-google-tv-hid
   ```
3. **Install required libraries:**
   - `BleKeyboard` or `Bluepad32` (depending on implementation)
4. **Flash to your ESP32 board**

## 🧠 How It Works

Google TV accepts standard Bluetooth HID devices, including keyboards. This project uses the ESP32's BLE capabilities to emulate a keyboard and send key combinations like:
- `Volume Up`: `KEY_MEDIA_VOLUME_UP`
- `Volume Down`: `KEY_MEDIA_VOLUME_DOWN`
- `Mute`: `KEY_MEDIA_MUTE`
- `Google Home Camera`: Shortcut key or sequence (e.g., `Ctrl+Alt+5`)

## 🔐 Permissions

No root or ADB required on the Google TV. Just pair your ESP32 as a Bluetooth keyboard and go.

## 📸 Screenshots / Video Demo

*Coming soon!*

## 🛠️ To-Do

- [ ] Add UI feedback on small OLED display
- [ ] Customizable key mapping via config file
- [ ] Voice trigger support

## 📄 License

MIT License — feel free to fork and modify.

---

Made with 💡 by [Your Name or Handle]
