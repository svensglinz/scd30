## SCD30 Sensor Interface Library & Web Service 

This project provides a simple way to interface with the Sensirion SCD30 sensor using a C library
and serve the sensor data via a lightweight Python web server.
It allows for easy integration with home automation tools or other systems that consume JSON APIs.

### Features

- **C library** to communicate with the SCD30 sensor via I2C (based on the official Sensirion datasheet [here](https://www.google.com)).
- A C program (`query_scd30`) that queries the sensor every 10 seconds and writes the data to shared memory.
- A minimal **Flask web** server that reads from shared memory and serves the sensor data as JSON:
{ "temp": float, "co2": float, "humidity": float }
- Automatic service startup via systemd
- One-step installation with an `install.sh` script

### Installation

1. Clone the repository
 
```bash
git clone git@github.com:svensglinz/scd30.git
cd ./scd30
```

2. Run the installation script

```bash
sudo ./install.sh
```

This script will: 
- Compile the C source file `query_scd30.c`
- Set up shared memory access 
- Install Python dependencies (Flask)
- Move and enable the systemd service
- Start the sensor querying and web server on boot

### Optional configs

The default webserver runs on port 8080. This can be changed by changing the `PORT` variable 
in the `install.sh` script before installation. 

### Usage 
...

### Requirements 
- I2C enabled and scd30 connected to your device
(the service looks for the I2C address 0x61 for the SCD30 sensor)
