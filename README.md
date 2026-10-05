# myOS — Complete Step-by-Step Guide

> **Hardware:** ESP32-S3 + ST7789 SPI Screen (320×240) + MAX98357A I2S Amplifier + PSRAM  
> **Framework:** ESP-IDF v5.x + FreeRTOS + LVGL  
> **OS:** Windows (PowerShell commands)

---

##  Table of Contents

1. [What You Need (Hardware + Software)](#1-what-you-need)
2. [Install ESP-IDF & VS Code](#2-install-esp-idf--vs-code)
3. [Wire Your Hardware](#3-wire-your-hardware)
4. [Open & Configure the Project](#4-open--configure-the-project)
5. [Milestone 1 — Boot + Screen](#5-milestone-1--boot--screen)
6. [Milestone 2 — WiFi](#6-milestone-2--wifi)
7. [Milestone 3 — Music Streaming](#7-milestone-3--music-streaming)
8. [Milestone 4 — Video Streaming](#8-milestone-4--video-streaming)
9. [Common Errors & Fixes](#9-common-errors--fixes)
10. [Project File Map](#10-project-file-map)

---

## 1. What You Need

### Hardware
| Item | Details |
|---|---|
| ESP32-S3 DevKit | DevKitC-1 (with PSRAM) — any 8MB flash variant |
| ST7789 SPI Screen | 320×240, 2.4" or 2.8" (non-touch or touch, both work) |
| MAX98357A Module | I2S amplifier + speaker (3W or 5W speaker) |
| Jumper wires | Female-to-male, ~20 pieces |
| USB cable | USB-C data cable (NOT charge-only) |
| Your laptop | Windows, same WiFi network as ESP32 for video |

### Software (install in this order)
| Software | Version | Download |
|---|---|---|
| Python | 3.10 or newer | https://www.python.org/downloads/ |
| Git | Latest | https://git-scm.com/download/win |
| VS Code | Latest | https://code.visualstudio.com/ |
| ESP-IDF Extension | v1.8.x | Inside VS Code (explained below) |

---

## 2. Install ESP-IDF & VS Code

### Step 2.1 — Install VS Code
Download and install VS Code from https://code.visualstudio.com/

### Step 2.2 — Install ESP-IDF Extension
1. Open VS Code
2. Press `Ctrl + Shift + X` to open Extensions
3. Search for **"ESP-IDF"** (by Espressif Systems)
4. Click **Install**
5. After install, press `Ctrl + Shift + P` → type **"ESP-IDF: Configure ESP-IDF Extension"**
6. Select **"Express"** install
7. Set IDF version to **v5.2 (recommended)**
8. Choose an install path (default is fine, e.g. `C:\Users\YourName\esp\esp-idf`)
9. Click **Install** and wait 10–20 minutes (it downloads toolchain + libraries)
10. When done you will see:  **"ESP-IDF is ready"**

> **Check:** Open a new Terminal in VS Code. Type `idf.py --version`  
> You should see: `ESP-IDF v5.2.x`

---

## 3. Wire Your Hardware

>  **Always wire with ESP32 UNPLUGGED from USB.**

### ST7789 Screen → ESP32-S3
| ST7789 Pin | Connect to | ESP32-S3 GPIO |
|---|---|---|
| VCC | 3.3V | 3V3 pin |
| GND | Ground | GND pin |
| SCL / SCK | GPIO **18** | D18 |
| SDA / MOSI | GPIO **19** | D19 |
| CS | GPIO **15** | D15 |
| DC / RS | GPIO **16** | D16 |
| RST / RES | GPIO **17** | D17 |
| BLK / LED | GPIO **14** | D14 |

### MAX98357A Amplifier → ESP32-S3
| MAX98357A Pin | Connect to | ESP32-S3 GPIO |
|---|---|---|
| VIN | 5V | 5V pin |
| GND | Ground | GND pin |
| BCLK | GPIO **7** | D7 |
| LRC / WS | GPIO **8** | D8 |
| DIN | GPIO **6** | D6 |
| SD (shutdown) | 3.3V (always on) | 3V3 pin |

### Speaker → MAX98357A
| Speaker Wire | MAX98357A Terminal |
|---|---|
| + (positive) | OUT+ |
| − (negative) | OUT− |

>  **Changed your pins?** Edit only `main/pins.h` — no other file needs changing.

---

## 4. Open & Configure the Project

### Step 4.1 — Open in VS Code
1. Open VS Code
2. Go to **File → Open Folder**
3. Select the `ESP32 Os` folder on your Desktop
4. Trust the workspace when asked

### Step 4.2 — Open a Terminal
Press `` Ctrl + ` `` (backtick) to open VS Code terminal.

### Step 4.3 — Set the Target Chip
```powershell
idf.py set-target esp32s3
```
Expected output: `Set Target to esp32s3`

### Step 4.4 — Download Library Dependencies
```powershell
idf.py update-dependencies
```
This downloads LVGL, ST7789 driver, and LVGL port from the component registry.  
Wait for: `All dependencies resolved`

### Step 4.5 — Check Your PSRAM Type
```powershell
idf.py menuconfig
```
Navigate using arrow keys:
```
Component config → ESP PSRAM → SPI RAM config → Type of SPI RAM chip
```
- **Octal** → select if your board has OPI PSRAM (DevKitC-1 default)
- **Quad** → select for older/budget boards

Press `S` to save, `Q` to quit menuconfig.

>  **Not sure?** Check your board's product page or datasheet. DevKitC-1 with "N16R8" in the name has Octal PSRAM.

---

## 5. Milestone 1 — Boot + Screen

**Goal:** Flash the firmware → screen lights up with "myOS" home screen showing 3 buttons.

### Step 5.1 — Plug in your ESP32
Connect ESP32 to your laptop via USB-C data cable.

### Step 5.2 — Find your COM port
```powershell
# Check Device Manager or run:
mode
```
Look for something like `COM3`, `COM4`, `COM5` etc.

### Step 5.3 — Build the project
```powershell
idf.py build
```
First build takes 3–5 minutes. Wait for:  
`Project build complete. To flash, run: idf.py flash`

>  **Build failed?** See [Common Errors](#9-common-errors--fixes) section below.

### Step 5.4 — Flash + Monitor
```powershell
idf.py -p COM3 flash monitor
```
Replace `COM3` with your actual port.

You will see logs like:
```
I (xxx) MAIN: === myOS booting ===
I (xxx) MAIN: NVS ready
I (xxx) DISPLAY: ST7789 panel up
I (xxx) DISPLAY: LVGL ready, display 320x240
I (xxx) MAIN: Launcher shown — boot complete
```

###  Milestone 1 PASS
**Screen shows:** Dark blue background + "myOS" title + 3 buttons (Music / Video / Settings)

To exit monitor: Press `Ctrl + ]`

---

## 6. Milestone 2 — WiFi

**Goal:** Connect to WiFi from the Settings app. No reflash needed — WiFi manager is already in the firmware.

### Step 6.1 — Open Settings App
On the screen, tap or press the **Settings** button.

> If no touch: add physical button on GPIO 0 (BOOT pin) as Back key, or wire a tactile switch.

### Step 6.2 — Enter Credentials
- Type your **WiFi SSID** in the SSID field
- Type your **WiFi password** in the password field
- Press **Connect**

### Step 6.3 — Verify in Monitor
Watch the serial monitor (`idf.py monitor`):
```
I (xxx) WIFI_MGR: Connecting to SSID: YourNetwork
I (xxx) WIFI_MGR: Got IP: 192.168.1.105
```
Status on screen turns green: **"Connected  IP: 192.168.x.x"**

###  Milestone 2 PASS
WiFi credentials are saved in flash (NVS). Next boot → auto-connects. No re-entry needed.

---

## 7. Milestone 3 — Music Streaming

**Goal:** Press Music → audio plays from an internet radio stream through the speaker.

### Step 7.1 — Install ESP-ADF (Audio Framework)

Open PowerShell **outside** VS Code (Win key → PowerShell):
```powershell
# Navigate to your ESP folder
cd C:\Users\YourName\esp

# Clone ESP-ADF (this takes a few minutes)
git clone --recursive https://github.com/espressif/esp-adf.git
```

### Step 7.2 — Set Environment Variable
```powershell
# Add this to your system environment variables permanently:
# Variable name:  ADF_PATH
# Variable value: C:\Users\YourName\esp\esp-adf

# Or set it for current session only:
$env:ADF_PATH = "C:\Users\YourName\esp\esp-adf"
```

To set permanently:
1. Press `Win + R` → type `sysdm.cpl` → Enter
2. Advanced → Environment Variables
3. System Variables → New
4. Name: `ADF_PATH`, Value: `C:\Users\YourName\esp\esp-adf`
5. OK → Restart VS Code

### Step 7.3 — Enable ADF in Project

**Edit `CMakeLists.txt`** (root file) — add this line before `include(project)`:
```cmake
include($ENV{ADF_PATH}/CMakeLists.txt)
```

**Edit `components/audio_svc/CMakeLists.txt`** — add ADF libs to REQUIRES:
```cmake
idf_component_register(
    SRCS "audio_svc.c"
    INCLUDE_DIRS "."
    PRIV_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/main"
    REQUIRES
        driver
        freertos
        esp_log
        esp_audio
        http_stream
        mp3_decoder
        i2s_stream
)
```

### Step 7.4 — Set Your Radio URL

Open `components/apps/music_app/music_app.c`, find line:
```c
#define DEFAULT_RADIO_URL \
    "http://icecast.radiofrance.fr/fip-lofi.mp3"
```
Change to any MP3 internet radio URL. Free radio stations:
- `http://icecast.radiofrance.fr/fip-lofi.mp3` (lofi, France)
- `http://stream.rcs.revma.com/ypqb04wr1d1uv` (relax radio)

### Step 7.5 — Rebuild & Flash
```powershell
idf.py fullclean
idf.py build flash monitor
```

###  Milestone 3 PASS
Press **Music** on launcher → speaker plays audio → Press Stop → audio stops.

---

## 8. Milestone 4 — Video Streaming

**Goal:** Laptop streams video over WiFi → ESP32 screen shows it at ~12 FPS.

### Step 8.1 — Install Python Library (on Laptop)
Open PowerShell:
```powershell
pip install opencv-python
```

### Step 8.2 — Get a Sample Video
Put any `.mp4` video file in the `tools/` folder. Rename it `sample.mp4`.  
Or use a different name and pass it with `--source`.

### Step 8.3 — Find Your Laptop's Local IP
```powershell
ipconfig
```
Look for **IPv4 Address** under your WiFi adapter:
```
IPv4 Address. . . . : 192.168.1.50   ← this is your laptop IP
```

### Step 8.4 — Start the Video Server
```powershell
cd "C:\Users\DELL PC\Desktop\ESP32 Os\tools"
python mjpeg_server.py --source sample.mp4
```
You will see:
```
[INFO] MJPEG server running on http://0.0.0.0:8080
[INFO] Enter your laptop IP in the ESP32 Video app
```

### Step 8.5 — Play on ESP32
1. On the launcher screen, tap **Video**
2. Enter your **laptop's IP** (e.g. `192.168.1.50`) in the IP field
3. Press  **Play**
4. Video plays on screen at ~12 FPS

>  **FPS too low?** Reduce JPEG quality or resolution:
> ```powershell
> python mjpeg_server.py --source sample.mp4 --fps 10
> ```

###  Milestone 4 PASS
Video visible on ESP32 screen. Press Stop → video stops.

---

## 9. Common Errors & Fixes

###  `idf.py: command not found`
ESP-IDF environment not loaded. In VS Code terminal:
```powershell
# The ESP-IDF extension auto-sources this. If not, run:
. $env:IDF_PATH\export.ps1
```

###  `No serial port found` / `Could not open COM3`
- Check USB cable (must be data cable, not charge-only)
- Check Device Manager for the correct COM port number
- Install CP2102 / CH340 driver if port not appearing

###  `LVGL: LV_COLOR_DEPTH not 16`
In `idf.py menuconfig`:
```
Component config → LVGL → Color settings → Color depth → 16
```

###  Screen blank / backlight off
- Check GPIO 14 connected to BL/LED pin of screen
- Swap DC and RST pins — easy to confuse
- Try lowering SPI clock: in `pins.h` change `40 * 1000 * 1000` to `20 * 1000 * 1000`

###  Screen shows garbled pixels / random colors
- Wrong `invert_color` setting for your screen
- In `display.c`, try changing `true` to `false`:
  ```c
  esp_lcd_panel_invert_color(s_panel, false);
  ```

###  `PSRAM alloc failed`
- PSRAM not enabled: run `idf.py menuconfig` → Enable PSRAM
- Wrong PSRAM type (Octal vs Quad): check your board specs

###  `heap_caps_malloc` returns NULL for video buffer
- Not enough PSRAM free. Stop audio before playing video.
- Reduce `JPEG_BUF_SIZE` in `video_svc.c` from 20KB to 12KB.

###  Audio crackles / stutters
- Increase audio task priority in `audio_svc.c`: change `5` to `6`
- Make sure audio task is on Core 1 (already set in code)

###  WiFi not connecting
- Check SSID/password (case-sensitive)
- WPA2 Enterprise networks (like university WiFi) not supported — use WPA2 Personal
- ESP32 only supports 2.4 GHz WiFi, not 5 GHz

###  `idf.py update-dependencies` fails
```powershell
# Try manually:
idf.py add-dependency "lvgl/lvgl>=8.3.0,<9.0.0"
idf.py add-dependency "espressif/esp_lvgl_port>=1.4.0"
idf.py add-dependency "espressif/esp_lcd_st7789>=1.2.0"
```

---

## 10. Project File Map

```
ESP32 Os/
│
├──  CMakeLists.txt          Root build file
├──  partitions.csv          Flash memory layout (8MB)
├──  sdkconfig.defaults      Pre-tuned settings (PSRAM, fonts, WiFi)
│
├──  main/
│   ├──  main.c              Boot sequence (NVS → Display → Services → Launcher)
│   ├──  pins.h               ALL GPIO numbers defined here — edit for your board
│   └──  idf_component.yml   Library dependency versions
│
├──  components/
│   ├──  display/
│   │   ├── display.h          API: display_init(), display_set_backlight()
│   │   └── display.c          ST7789 SPI init + LVGL port setup
│   │
│   ├──  wifi_mgr/
│   │   ├── wifi_mgr.h         API: wifi_mgr_connect(), is_connected(), get_ip()
│   │   └── wifi_mgr.c         Station mode, NVS save/load, auto-reconnect
│   │
│   ├──  audio_svc/
│   │   ├── audio_svc.h        API: audio_svc_play(url), stop(), set_volume()
│   │   └── audio_svc.c        FreeRTOS queue + ESP-ADF pipeline (Core 1)
│   │
│   ├──  video_svc/
│   │   ├── video_svc.h        API: video_svc_play(url, canvas), stop()
│   │   └── video_svc.c        MJPEG HTTP stream + JPEG decode → LVGL canvas
│   │
│   └──  apps/
│       ├──  launcher/       Home screen — 3 app buttons
│       ├──  music_app/      Music player UI (play/stop/volume)
│       ├──  video_app/      Video player UI (IP input + canvas)
│       └──  settings_app/   WiFi SSID/password input + status
│
└──  tools/
    └──  mjpeg_server.py     Run on laptop to stream video to ESP32
```

---

##  Quick Command Reference

```powershell
# Set chip target (once)
idf.py set-target esp32s3

# Download libraries (once, or after adding dependencies)
idf.py update-dependencies

# Open config menu
idf.py menuconfig

# Build only
idf.py build

# Build + Flash + Monitor (replace COM3 with your port)
idf.py -p COM3 flash monitor

# Monitor only (already flashed)
idf.py -p COM3 monitor

# Full clean rebuild (when something is broken)
idf.py fullclean
idf.py build

# Exit monitor
Ctrl + ]

# Start video server on laptop
python tools/mjpeg_server.py --source sample.mp4 --fps 12
```

---

##  Milestone Checklist

| Milestone | What to verify | Status |
|---|---|---|
|  M1: Screen | Launcher home screen visible with 3 buttons | |
|  M2: WiFi | Settings app → connect → green IP shown | |
|  M3: Music | Music button → audio from speaker | |
|  M4: Video | Video button → enter laptop IP → video plays | |

---

*Built with ESP-IDF v5.x • LVGL 8.x • FreeRTOS • ESP-ADF*
