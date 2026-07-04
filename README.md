# MineGuard Wokwi Prototype

This Wokwi prototype uses the ModestIoT framework as an external Wokwi library.
Do not add `Device.h`, `Sensor.h`, `Actuator.h`, `Led.h`, `Button.h`, or other
framework files as sketch tabs. They are loaded from `libraries.txt`.

wokwi : https://wokwi.com/projects/467328858128287745 

## Required Wokwi Libraries

```text
ArduinoJson
PubSubClient
ESP32Servo
LiquidCrystal I2C
DHT sensor library
Adafruit Unified Sensor
EspSoftwareSerial
HX711 Arduino Library
ModestIoT@wokwi:fb939e3848beed149e380d7392ca7967aa440c62
```

## Pin Map

| Module | ESP32 pins |
| --- | --- |
| Front distance sensor | TRIG `D5`, ECHO `D18` |
| Back distance sensor | TRIG `D19`, ECHO `D34` |
| Left distance sensor | TRIG `D25`, ECHO `D35` |
| Right distance sensor | TRIG `D26`, ECHO `D27` |
| Pulse sensor | `D32` |
| Collision button | `D4` |
| SOS button | `D13` |
| Buzzer | `D23` |
| Red LED | `D15` |
| Green LED | `D2` |
| LCD I2C | SDA `D21`, SCL `D22` |
| GPS | RX2 `D16`, TX2 `D17` |

The proximity alert uses the nearest value from the four distance sensors and
activates only at `20 cm` or less.
