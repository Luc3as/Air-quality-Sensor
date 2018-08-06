# Air quality sensor
This simple, fancy looking, ESP8266 based sensor measures values of CO2 and TVOC air pollutants.
As output there is addressable RGB led strip, and/or optional OLED display which can show real time levels.

## Material needed
* [Wemos D1 mini](https://www.aliexpress.com/item/ESP8266-ESP12-ESP-12-WeMos-D1-Mini-WIFI-Dev-Kit-Development-Board-NodeMCU-Lua/32653918483.html?ws_ab_test=searchweb0_0,searchweb201602_4_10152_10151_10065_10068_10344_10342_10343_10340_10341_10696_10084_10083_10618_10307_10820_10821_10303_10846_10059_100031_524_10103_10624_10623_10622_10621_10620,searchweb201603_25,ppcSwitch_5&algo_expid=7c525067-46ab-4a80-baab-f36e6cb5dd58-0&algo_pvid=7c525067-46ab-4a80-baab-f36e6cb5dd58&transAbTest=ae803_1&priceBeautifyAB=0)
* [CCS811 sensor](https://www.aliexpress.com/item/CJMCU-811-CCS811-Carbon-Monoxide-CO-VOCs-Air-Quality-Numerical-Gas-Sensors/32784803629.html?ws_ab_test=searchweb0_0,searchweb201602_4_10152_10151_10065_10068_10344_10342_10343_10340_10341_10696_10084_10083_10618_10307_10820_10821_10303_10846_10059_100031_524_10103_10624_10623_10622_10621_10620,searchweb201603_25,ppcSwitch_5&algo_expid=90061718-43a7-4c82-83e5-2a1a41b8e246-0&algo_pvid=90061718-43a7-4c82-83e5-2a1a41b8e246&transAbTest=ae803_1&priceBeautifyAB=0)
* [WS2812b led strip](https://www.aliexpress.com/item/1m-4m-5m-WS2812B-Smart-led-pixel-strip-Black-White-PCB-30-60-144-leds-m/2036819167.html?ws_ab_test=searchweb0_0,searchweb201602_4_10152_10151_10065_10068_10344_10342_10343_10340_10341_10696_10084_10083_10618_10307_10820_10821_10303_10846_10059_100031_524_10103_10624_10623_10622_10621_10620,searchweb201603_25,ppcSwitch_5&algo_expid=e920b19e-1960-4e49-b688-bdefa83688ed-0&algo_pvid=e920b19e-1960-4e49-b688-bdefa83688ed&transAbTest=ae803_1&priceBeautifyAB=0)
* [Optional OLED display](https://www.aliexpress.com/item/Free-Shipping-White-Blue-Whiteand-Blue-color-0-96-inch-128X64-OLED-Display-Module-For-arduino/32713614136.html?ws_ab_test=searchweb0_0,searchweb201602_4_10152_10151_10065_10068_10344_10342_10343_10340_10341_10696_10084_10083_10618_10307_10820_10821_10303_10846_10059_100031_524_10103_10624_10623_10622_10621_10620,searchweb201603_25,ppcSwitch_5&algo_expid=a29b31fa-7b49-45dd-bc4d-7a5f6978be6e-2&algo_pvid=a29b31fa-7b49-45dd-bc4d-7a5f6978be6e&transAbTest=ae803_1&priceBeautifyAB=0)


And of course some soldering iron and generic tools.

### 3D printed case
You can download latest version of case on [thingiverse](https://www.thingiverse.com/thing:2997734).
There will be updated side cover with hole for display soon. 

## Schematic

![Connection diagram](https://github.com/Luc3as/Air-quality-Sensor/blob/master/Docs/Air%20quality%20sensor_bb.png?raw=true)

## Final sensor assembly
![wiring](https://github.com/Luc3as/Air-quality-Sensor/blob/master/Docs/4.jpg?raw=true)
![1](https://github.com/Luc3as/Air-quality-Sensor/blob/master/Docs/1.jpg?raw=true)
![2](https://github.com/Luc3as/Air-quality-Sensor/blob/master/Docs/2.jpg?raw=true)
![3](https://github.com/Luc3as/Air-quality-Sensor/blob/master/Docs/3.jpg?raw=true)


## Configuration
There is source code attached, I am using WiFi Manager, so after burning firmware, there will be WiFi hotspot, you can connect with password "config123".
After that you should open address 192.168.4.1 and there will be configuration manager where you can enter details about your SSID, password, MQTT broker address, port, username and password, as well as MQTT topic where should sensor publish the data.

I connected 4 pieces of WS2812b LEDS, if you have different number, you should edit following line in main.cpp and compile project in platformio.
```c++
#define NUMPIXELS      4
```

## Measuring
I don't know it I received bad piece of CCS811 sensor, but I spent few nights discovering why it stops reading data after few hours of operation, and only  thing that helped to recover it was to unplug power source and plug it back.
I realized that in default it takes reading every second. So I found out how to set to take reading every 10 seconds with line
```c++
ccs.setDriveMode(CCS811_DRIVE_MODE_10SEC);
```
I am taking 6 readings, calculate average and then print and publish data, so sensor sends data every minute.

## Burning the firmware
I precompiled firmware for Wemos D1 mini you can download it here [![GitHub version](https://img.shields.io/github/release/Luc3as/Air-quality-Sensor.svg)](https://github.com/Luc3as/Air-quality-Sensor/releases/latest)

You can use attached burner from easy esp, you just have to select COM port of ESP and select burn.

![Burning the firmware](https://github.com/Luc3as/Air-quality-Sensor/blob/master/Docs/burning.png?raw=true)

## HomeAssistant example configuration
This example shows sensor configuration for reading ppm values, and it shows other values as json attributes so it can be viewed after opening sensor details. 
```python
# CO2 sensor
  - platform: mqtt
    state_topic: 'stat/air-quality-monitor'
    name: 'CO2 sensor'
    unit_of_measurement: 'ppm'
    value_template: '{{ value_json.eCO2 }}'
    json_attributes:
      - TVOC
      - temperature
```

