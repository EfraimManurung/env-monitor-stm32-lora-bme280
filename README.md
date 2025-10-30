# env-monitor-stm32-lora-bme280

Author: Efraim Manurung
email: efraim.manurung@gmail.com

Generated with ChatGPT with guidance from the author.

A low-power environmental sensor node built with an STM32-based MiniPill LoRa board. This node is capable of measuring temperature and humidity using a BME280 sensor and transmitting the data over LoRa, making it ideal for applications such as weather stations, greenhouse monitoring, or other remote sensing deployments.

I refactored to RadioLib and will be implented as an IoT system. I refactored to RadioLib and will be implemented as an IoT system. Further information will be announced. 

## ✨ Features

- 🧠 Based on STM32 "MiniPill" LoRa board the work of Leo Korbee ([iot-lab.org](https://www.iot-lab.org/about/))
- 🌡️ Measures temperature and humidity with BME280. 
- 📡 LoRa data transmission using RadioLib.  
- 🔋 Designed for low power consumption.  
- 🌿 Suitable for outdoor and greenhouse environments.  

## 🛠 Hardware

- **Board**: [MiniPill LoRa STM32 Board](https://www.iot-lab.org/blog/370/)  
- **MiniPill_LoRa_Board**: [Boards and Variants](https://gitlab.com/iot-lab-org/minipill_lora_board)
- **Sensor**: BME280 (temperature & humidity)  
- **Radio**: LoRa RFM95/SX1276 via RadioLib  
- **Please read the documentation from the docs for the datasheets**

## 📦 Libraries Used

- [RadioLib](https://github.com/jgromes/RadioLib) – LoRa communication  
- [BME280 by bolderflight](https://github.com/bolderflight/BME280) – Sensor driver  
- [STM32duino](https://github.com/stm32duino) – STM32 core and board support  

## 🔧 Programming the Board

Refer to the official [MiniPill programming guide](https://www.iot-lab.org/blog/355/) for flashing firmware and setting up your environment.

## 👣 How to Flash It

This project supports two modes: **transmit** and **receive**, each with its own entry point:

- `main_transmit.cpp` – for sending sensor data via LoRa
- `main_receive.cpp` – for receiving data via LoRa

You can flash either mode using PlatformIO from the terminal.

### 🔧 Transmit Mode

```bash
# flash transmit
pio run -e transmit            # Build only
pio run -e transmit -t upload  # Build and upload to the board

# flash receive
pio run -e receive             # Build only
pio run -e receive -t upload   # Build and upload to the board


```

## 🚀 Getting Started

1. Connect your BME280 sensor to the MiniPill board (I2C).
2. Install the required libraries listed above.
3. Flash the firmware to the MiniPill using STM32CubeProgrammer or PlatformIO.
4. Power the device and start collecting data.

## 🚀 Results

### Power Consumption Measurements

| Mode | Description | Current (µA) |
|------|--------------|--------------|
| Measuring Mode | STM32L051C8T6 + BME280 (temperature and humidity) | **2.7 µA** |
| Low Power Mode | STM32L051C8T6 (with CR2032, 240 mAh) | **1.7 µA** |

Below are the experimental measurements using a multimeter:

![Power Measurement Results](docs/power-measurements-results.PNG)

These results demonstrate the **ultra-low power performance** of the STM32L051C8T6-based sensor node, making it highly efficient for long-term, battery-powered IoT deployments such as remote environmental monitoring.

## 📄 License

This project is licensed under the MIT License.
