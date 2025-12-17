# Heartbeat Detection Using Zephyr RTOS

A real-time heartbeat detection system built on Zephyr RTOS with PPG (photoplethysmography) sensor signal processing and biometric analysis.

---

## About the Project

This project implements a biometric heartbeat detection system using the Zephyr RTOS. The system acquires analog signals from a PPG sensor, processes the data in real-time to extract heart rate information, and provides accurate BPM (beats per minute) measurements. The application demonstrates Zephyr's capabilities for low-latency signal processing and real-time operations.

---

## Technologies Used

- Zephyr RTOS
- C programming language
- CMake build system
- West build tool
- ESP32 microcontroller
- ADS1115 high-precision ADC
- AD8232 ECG (Electrocardiogram) signal conditioning amplifier
- Real-time biomedical signal processing

---

## Prerequisites

Before getting started, ensure you have the following installed:

- Python 3.8 or higher
- Git
- CMake 3.13.1 or higher
- A supported ARM Cortex toolchain
- Zephyr SDK
- West tool (Zephyr's meta-tool)

---

## Hardware Setup

### Components

**ESP32 Microcontroller:** The main processing unit running Zephyr RTOS. Handles data acquisition, signal processing, and communication.

**AD8232 ECG Signal Conditioning Amplifier:** A specialized integrated circuit designed for biomedical signal acquisition. It:
- Amplifies weak ECG signals from body electrodes
- Removes DC offset and 60/50 Hz noise
- Provides a high-quality analog signal suitable for conversion
- Includes lead-off detection to identify poor electrode contact

**ADS1115 ADC (Analog-to-Digital Converter):** A high-precision 16-bit I2C ADC that:
- Converts the analog signal from the AD8232 to digital format
- Provides programmable gain amplification
- Supports 4 differential or single-ended input channels
- Communicates with the ESP32 via I2C protocol
- Allows multiple devices on the same I2C bus

---

### Connection Diagram 

<img width="804" height="992" alt="hardware_ecg drawio" src="https://github.com/user-attachments/assets/dd3ed273-2a57-4c54-9187-514f64c4022d" />

#### ADS1115 Connections

| Component  | Module Pin     | Connects to   | Function                |
| ---------- | -------------- | ------------- | ----------------------- |
| ADS1115    | SDA            | GPIO 21 ESP32 | I²C – Dados             |
| ADS1115    | SCL            | GPIO 22 ESP32 | I²C – Clock             |
| ADS1115    | ADDR           | GND ESP32     | Ground Connection       |
| ADS1115    | A0             | OUTPUT AD8232 | Analogic Input (ECG)    |
| ADS1115    | ADDR           | GND ESP32     | Ground Connection       |
| ADS1115    | GND            | GND ESP32     | Ground Connection       |
| ADS1115    | VCC            | 3V3 ESP32     | Power Connection        |

#### AD8232 Connections

| Component  | Module Pin     | Connects to   | Function            |
| ---------- | -------------- | ------------- | ------------------- |
| AD8232     | LO+            | GPIO 26 ESP32 | Lead-off positive   |
| AD8232     | LO-            | GPIO 27 ESP32 | Lead-off negative   |
| AD8232     | GND            | GND ESP32     | Ground Connection   |
| AD8232     | VCC            | 3V3 ESP32     | Power Connection    |

---

### Electrode Placement

<img width="434" height="399" alt="image" src="https://github.com/user-attachments/assets/26760abb-d10c-425c-914e-8ffe4258ecb2" />

Ensure good skin contact for accurate signal acquisition.

---

## Installing Zephyr RTOS

Follow these steps to set up Zephyr on your system:

### Step 1: Install West

West is Zephyr's meta-tool that manages the repository and build system.

```bash
pip install --upgrade west
```

### Step 2: Initialize Zephyr Workspace

Create a workspace directory and initialize it with Zephyr:

```bash
mkdir zephyr-workspace
cd zephyr-workspace
west init -m https://github.com/zephyrproject-rtos/zephyr.git --mr main
```

This creates the following structure:
- `.west/`: Meta-tool configuration
- `zephyr/`: Zephyr kernel source code
- `modules/`: Additional Zephyr modules

### Step 3: Update Modules

Fetch and update all dependencies:

```bash
west update
```

### Step 4: Install Zephyr SDK

Download and install the Zephyr SDK for your platform:

```bash
cd ~
wget https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.15.2/zephyr-sdk-0.15.2_linux-x86_64.tar.gz
tar xvf zephyr-sdk-0.15.2_linux-x86_64.tar.gz
zephyr-sdk-0.15.2/setup.sh
```

For Windows or macOS, download the appropriate installer from the Zephyr SDK releases.

### Step 5: Set Up Environment

Source the Zephyr environment:

```bash
cd ~/zephyr-workspace
source zephyr/zephyr-env.sh
```

For permanent setup, add this to your shell profile (.bashrc, .zshrc, etc.).

---

## Building the Project

### Clone the Repository

```bash
cd ~/zephyr-workspace
git clone https://github.com/cyberdebb/Heartbeat-Detection-Using-Zephyr-RTOS.git heartbeat-app
cd heartbeat-app
```

### Configure Your Target Board

This project is specifically designed for the ESP32 board. Use:

```bash
west build -b esp32
```

If using a specific ESP32 variant, select the appropriate board name:

- `esp32` - Generic ESP32
- `esp32s3_devkitc` - ESP32-S3
- `esp32c3_devkitm` - ESP32-C3

### Build with West

Build the application for the ESP32:

```bash
west build -b esp32
```

For a clean rebuild:

```bash
west build -b esp32 --pristine
```

### Build Output

The compiled firmware will be located in:

```
build/zephyr/zephyr.elf
build/zephyr/zephyr.hex
build/zephyr/zephyr.bin
```

---

## Flashing the Device

After a successful build, flash the firmware to your microcontroller:

```bash
west flash
```

The command automatically detects the connected device and uploads the firmware.

For specific device selection:

```bash
west flash --runner jlink
```

Supported runners include: jlink, pyocd, openocd, nrfjprog, and others.

---

## Monitoring Serial Output

View real-time debug output from the device:

```bash
west esplog
```

Or use a serial terminal:

```bash
screen /dev/ttyACM0 115200
```

Replace `/dev/ttyACM0` with your device's serial port.

---

## Project Configuration

### prj.conf

Main Zephyr configuration file. Key settings for this project:

- ADC peripheral configuration for sensor input
- Thread/task priorities and stack sizes
- Logging configuration level
- Device tree overlay settings

### app.overlay

Device tree overlay file that configures:

- ADC channel mapping for PPG sensor
- GPIO pins for status indicators
- UART/serial interface settings
- Hardware timer configuration

### CMakeLists.txt

Build configuration specifying:

- Source files to compile
- Include directories
- Library dependencies
- Zephyr-specific settings

---

## User Application (src)

The src directory contains the main application code:

- I2C communication driver for ADS1115 ADC interaction
- ADC sampling configuration for reading digitized ECG signals
- Real-time ECG signal processing from the AD8232 conditioning circuit
- Heart rate detection algorithm analyzing the digitized waveform
- Thread-based concurrent acquisition and processing tasks
- Lead-off detection for electrode contact monitoring
- Serial communication for outputting heart rate and diagnostic data

The application uses Zephyr threads to run simultaneous tasks: one reading from the ADS1115 via I2C and storing samples, another processing the signal buffer to detect QRS complexes and calculate BPM. This demonstrates Zephyr's real-time capabilities for biomedical signal processing.

---

## Features

- Real-time ECG signal acquisition via ADS1115 I2C ADC
- AD8232 biomedical signal conditioning for accurate measurements
- Automatic heartbeat detection and BPM calculation from QRS complexes
- Low-latency signal processing using Zephyr threads
- Lead-off detection for electrode contact quality monitoring
- Configurable sampling rates and detection thresholds
- Serial output of heart rate and diagnostic information
- Minimal CPU and power overhead for wearable applications

---

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for more details.
