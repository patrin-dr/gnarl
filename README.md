# GNARL is Not A RileyLink

## This fork: Heltec WiFi LoRa 32 V2 (868 MHz)

Branch `heltec-v2` ports GNARL to the **Heltec WiFi LoRa 32 V2 / V2.1**
(ESP32 + SX1276, 868 MHz) and **V3.x with an external RFM95W** for a
Medtronic MiniMed 722 WW pump with AndroidAPS.

Supported boards (choose with `make gnarl BOARD=...`, default `heltec_v2`):

| Board | `BOARD=` | Radio |
| --- | --- | --- |
| Heltec WiFi LoRa 32 V2 / V2.1 (ESP32) | `heltec_v2` | on-board SX1276 |
| Heltec WiFi LoRa 32 V3 / V3.1 / V3.2 (ESP32-S3) | `heltec_v3_rfm95` | **external RFM95W 868 MHz** |

Heltec V3.x boards carry an SX1262, which has no OOK and cannot talk to
Medtronic pumps. Wire an RFM95W (SX1276) module to the V3 header instead;
the on-board SX1262 is put to sleep at start-up:

| RFM95W | Heltec V3 (silkscreen label) |
| --- | --- |
| 3.3V | 3V3 |
| GND | GND |
| SCK | GPIO5 |
| MISO | GPIO7 |
| MOSI | GPIO6 |
| NSS / CS | GPIO4 |
| RESET | GPIO2 |
| DIO2 | GPIO38 |

DIO0 and DIO1 are not needed. Pins and other board details live in
`include/boards/`. CI builds every program for both boards; firmware for
V3 is named `<program>-heltec_v3_rfm95-firmware`.

Changes compared to upstream:

* **Board pins** in `include/module.h`: LED on GPIO25, OLED power via
  Vext (GPIO21, active LOW), SX1276 DIO1/DIO2 on GPIO35/GPIO34
  (TTGO v1 uses GPIO33/GPIO32). Set `HELTEC_V2_1` to match your board
  revision (printed on the board: "V2" or "V2.1").
* **Robustness**: no reboot after the phone disconnects, no `assert()`
  crashes on BLE errors, length checks on all rfspy commands, error
  replies instead of silence, watchdog reset while idle.
* **Power**: dynamic CPU frequency (40-80 MHz), radio Sleep after
  transmit-only commands, slower BLE advertising while disconnected.
  Small `sdkconfig` changes live in `src/<project>/sdkconfig.overrides`.
* **Battery**: voltage sensing, standard BLE Battery Service
  (0x180F) for AndroidAPS, battery level on the OLED, deep sleep below
  `BATTERY_CUTOFF_MV` to protect the cell. Calibrate with
  `BATTERY_CALIBRATION_PCT` against a multimeter.
* **Phone lock** (`BLE_LOCK_TO_PHONE`): the RileyLink characteristics
  require an encrypted, bonded link. The first phone that pairs ("Just
  Works", Android asks once to pair with GNARL) becomes the owner; any
  other device is disconnected. Hold **PRG for 5 s** to forget the phone,
  then also "Forget" GNARL in the phone's Bluetooth settings and pair again.
* **Boot screen**: firmware version (`git describe`), pump frequency,
  TX amplifier mode and pairing state, shown for 4 s after power-on.
* **Display language** `DISPLAY_LANG_RU` (on by default): Russian text with
  Cyrillic fonts; comment out for English. All strings are in
  `src/gnarl/display_text.h`. `tools/screen_preview/run.sh ru|en` renders
  every screen on a computer (CI job `screens` does it on each push and
  fails if text does not fit).
* **BLE transmit power** `BLE_TX_POWER` (default 0 dBm) — raise it if the
  phone connection drops.
* **TX amplifier**: `LORA_TX_PA_BOOST` / `LORA_TX_POWER_DBM` (default
  PA_BOOST, +10 dBm; upstream used the RFO output).
* **Logging**: INFO by default; uncomment `GNARL_DEBUG` in
  `src/gnarl/gnarl.h` for packet dumps.
* **CI**: GitHub Actions builds GNARL and the test apps with ESP-IDF
  v5.0.7 on every push; each run has `<program>-firmware` artifacts that can
  be flashed with `esptool` without installing ESP-IDF:

      python3 -m esptool --chip auto -p <port> -b 460800 write_flash @flash_args

  (run in the unpacked artifact directory).
* **Releases**: pushing a tag such as `v0.2.0` publishes a GitHub Release
  with a zip for every program (`gnarl`, `blink`, `regtest`, `oledtest`,
  `mmtune`, `pumpstat`), each with a `FLASHING.txt`. Artifacts of normal
  builds expire after 90 days; releases are kept.

This is DIY software for an insulin pump. Test in open loop first, keep
the pump's own alarms on, and always have a manual fallback.

---

![TTGO LoRa OLED v1](images/ttgo.png)

## Warning

This repository contains **very preliminary** code, intended for
collaboration among developers. It **is not ready** for end users
and may be subject to rebasing without notice.

## Hardware

This project has been developed and tested on
a TTGO version 1 ESP32 868/915 MHz LoRa OLED module,
which contains an ESP32 SoC, an RFM95 LoRa radio,
a 128x64 pixel SSD1306 OLED display, and a LiPo battery charger. The
version without OLED display is supported as well (see below in "Software
Setup").

The module has two push-buttons.
One is hard-wired to reset the board;
the other is available as an input or interrupt source.

They are available from [AliExpress,](https://www.aliexpress.com/item/2pcs-TTGO-LORA32-Lora-868Mhz-ESP32-LoRa-OLED-0-96-Inch-Blue-Display-Bluetooth-WIFI-ESP/32839249834.html)
[Banggood,](https://www.banggood.com/2Pcs-Wemos-TTGO-LORA32-868915Mhz-ESP32-LoRa-OLED-0_96-Inch-Blue-Display-p-1239769.html)
and numerous others.

### *TTGO version 2 module is not supported*

The current code does not work on the TTGO version 2 module due to
different pin assignments.

## Radios

The ESP32 SoC contains both WiFi and Bluetooth radios.

The radio chip ([HopeRF RFM95](https://www.hoperf.com/modules/lora/RFM95.html) /
[Semtech SX1276](https://www.semtech.com/products/wireless-rf/lora-transceivers/sx1276))
is marketed for LoRa applications, but also supports OOK and FSK modulation.
The OOK capability is used by this project to communicate with Medtronic insulin pumps.
It may also be possible to use the FSK support and a 433 MHz version
of the module to communicate with OmniPod insulin pumps:
if anyone pursues that, please let me know.

## Hardware Setup

### Antenna

Attach an appropriate antenna to the U.FL connector on the module
before using this software.

### Power

The module can be powered via the micro-USB connector or with a 3.7V
LiPo battery.

The battery connects to a 2-pin female JST 1.25mm connector.
Note that this is smaller than the 2.0mm connectors used on Adafruit and
SparkFun LiPo batteries.

## Software Setup

### Initialize git submodules

This repository contains git submodules (currently just one, in the `lib` subdirectory).
Unless you cloned this repository with the `--recursive` option,
those submodules won't be initialized yet.
Use this command to initialize them:
```
git submodule update --init --recursive
```

and this command to check their status:
```
git submodule status --recursive
```

### Set up a local ESP32 development environment

1. [Follow these instructions to install the ESP-IDF development environment.](https://docs.espressif.com/projects/esp-idf/en/v5.0/esp32/get-started/index.html#installation-step-by-step)

1. Build and flash one of the example applications to make sure you have a working setup.

### Alternative: Use the ESP-IDF Docker image

1. Run `docker pull espressif/idf:v5.0.7` to download the ESP-IDF image.
[See this page for additional information.](https://docs.espressif.com/projects/esp-idf/en/v5.0/esp32/api-guides/tools/idf-docker-image.html)

### Disable OLED support if necessary

OLED use is defined in `include/module.h` and must be commented out if no OLED is available:

	// #define OLED_ENABLE

## Building GNARL

1. Type `make` in the top level of this repository.

1. Change to the `project` subdirectory.

1. If using Docker, start an interactive ESP-IDF container for the build by running
`docker run -it --rm -v $PWD:/project -w /project --device /dev/ttyUSB0 espressif/idf:v5.0.7`
(USB passthrough does not work with Docker on macOS: build in Docker, flash with `esptool`).

1. Build the project by running `idf.py build`.
Do **not** run `idf.py set-target`: it regenerates `sdkconfig` and discards the project configuration.

1. Flash the project to your ESP32 module by running `idf.py flash`

## Running GNARL

Run [Loop](https://loopkit.github.io/loopdocs/) on your iPhone
or run [AndroidAPS](https://github.com/nightscout/AndroidAPS) on your Android phone.
GNARL should show up when you scan for a RileyLink.

GNARL will show messages on the OLED display when your phone
connects and disconnects. Pushing the button will display the current
status. GNARL may not respond to the button press immediately if it is
communicating with the pump, due to scheduling priorities.

## Building the other applications

This repository contains a few applications ("projects" in ESP-IDF terminology) besides GNARL:

* `bletest` is a simple BLE server that can be queried with a tool like
  [nRF Connect](https://www.nordicsemi.com/Software-and-Tools/Development-Tools/nRF-Connect-for-mobile)

* `blink` blinks the onboard LED

* `mmtune` finds the frequency at which the pump responds
  with the strongest signal and displays the results graphically

* `nightscout` retrieves recent entries from a Nightscout server and
  prints them on the serial console

* `oledtest` draws text in various sizes on the OLED display

* `pumpclock` retrieves the time from a Medtronic insulin pump and
  uses it to display a digital clock

* `pumpstat` displays the status of a Medtronic insulin pump when you press the button

* `regtest` reads the SX1276 (RFM95) registers and prints them on the serial console;
  register `42` (RegVersion) should read `12`

* `sleep` uses the ESP32 "lightweight sleep" mode and wakes up when a
  timer goes off or the button is pressed

* `sniffer` receives Medtronic packets and prints them on the serial console

* `wifi` connects to a WiFi network, obtains an IP address, and prints
  it on the serial console

To build the `blink` application, for example:

1. In the top level of this repository, type `make clean && make blink`

1. Change to the `project` directory

1. Follow the same steps as above for building and flashing

After flashing applications that print information on the serial console,
run `idf.py monitor` to see the output.

### Pump-specific configuration

Some of the applications require the pump serial number or frequency
to be defined in the `include/pump_config.h` file.
It should look like this:

	#define PUMP_ID		"123456"	// pump serial number (note that this is a string constant)
	#define PUMP_FREQUENCY	916500000	// pump frequency
	#define MMTUNE_START	916300000	// starting frequency for mmtune scans

For worldwide (WW) pumps use 868 MHz values, e.g. `868350000` and `868200000`.

### Network configuration

The network configuration for WiFi or Bluetooth tethering must be hard-coded in the `include/network_config.h` file.
For WiFi, define the following:

	#define WIFI_SSID	"network name"
	#define WIFI_PASSWORD	"network password"

For Bluetooth tethering, define the following:

	#define USE_BLUETOOTH_TETHERING
	#define TETHER_ADDRESS	"12:34:56:78:9A:BC"

The Bluetooth tethering code currently requires
[the BlueKitchen Bluetooth stack.](https://github.com/bluekitchen/btstack)
Follow [these instructions](https://github.com/bluekitchen/btstack/blob/master/port/esp32/README.md)
to integrate it into your ESP-IDF environment.
If you are using Docker, start your Docker container with a bind-mount for the btstack source tree, so you can integrate it once the container is running.

### Nightscout configuration

Nightscout server information must be defined in `include/nightscout_config.h` as follows:

	// DNS hostname, not a URL
	#define NIGHTSCOUT_HOST		"your.nightscout.hostname"

	// 40-character SHA-1 hash of your Nightscout API secret
	#define NIGHTSCOUT_API_SECRET	"0123456789abcdef0123456789abcdef01234567"

The SSL layer requires the root certificate used by the Nightscout
server to be available at compile time in the file `include/root_cert.pem`.
You can extract it from the output of this command:

    openssl s_client -showcerts -connect NIGHTSCOUT_HOST:443 </dev/null

The root certificate is the last one in the chain.

### Papertrail configuration

Papertrail logging information must be defined in `include/papertrail_config.h` as follows:

	#define PAPERTRAIL_HOST	"xyz.papertrailapp.com"
	#define PAPERTRAIL_PORT	12345

### Time zone configuration

The local time zone must be hard-coded in the `include/timezone_config.h` file.
This fork uses Moscow time (UTC+3, no daylight saving time):

    #define TZ	"MSK-3"

Note that POSIX inverts the sign: `MSK-3` means three hours *ahead* of UTC.
Other examples: `EST5EDT,M3.2.0,M11.1.0` (US Eastern), `CET-1CEST,M3.5.0,M10.5.0/3` (Central Europe).

The time zone must be in one of the first two formats
[specified here.](https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html)
Unfortunately, the friendly "Europe/Moscow" format is *not* supported.

Only the `nightscout` and `xdrip` helper applications use the time zone;
GNARL itself does not (AndroidAPS manages the pump clock).
