# ESPHome component for Samsung AC

Should be compatible with models Samsung AR24HSFSAWKN and other models using the same protocol.

Samsung air conditioner has an option for connecting remote module purchased separately. The module was from around 2010 and the App is not maintained anymore. This project aims to replace this module with more affordable and universal ESP module and to allow integration to home automation systems like HomeAssistent.

This component use ESPHome UART to connect with Samsung AC and communicates directly with Home Assistant.

### Supported Samsung units

Any unit which have an option to purchase o wifi adapter DB68-04538A:

* AR24HSFSAWKN

## Hardware

* tested with ESP32c3 
* connection adapter need to be build using a level shifter between 5V (AC unit) and 3.3V (ESP32) (search Aliexpress for "level shifter"). That's what I use and it works fine.
* the AC connector deliver 12V, If you want to power the device from there you'll also need a step down converter 12v -> 5V.

* CN51 is a HY6P 2.0mm (https://fr.aliexpress.com/item/1005008289279869.html)
* I don't know what exactly the terminal at the other end is.

As owned an obsolete DB68-04538A wifi module I reused the included step down converter.

### ESP32c3

   ![schema](/images/schema.jpg)
   ![assembly1](/images/assembly1.jpg)
   ![assembly2](/images/assembly2.jpg)
   ![assembly3](/images/assembly3.jpg)
   ![assembly4](/images/assembly4.jpg)
   ![assembly5](/images/assembly5.jpg)
   ![assembly6](/images/assembly6.jpg)

### Pinout

AC unit has a wifi connector CN51 with an extension cable, a gray one with 6 pins.

<p align="center">

|pin number| color  | ESP32 pin  |
|----------|--------|------------|
|    6     | green  | unused     |
|    5     | yellow | 12v        |
|    4     | orange | GND        |
|    3     | black  | RST (unused) |
|    2     | pink   | TX         |
|    1     | red    | RX         |
</p>

   ![stepdown](/images/step_down.png)

## Installation

Create a yaml config file and setup UART and Climate modules. See [example.yaml](https://github.com/kumy/esphome_samsung_ac/blob/main/example.yaml) for configuration details.

```yaml
[...]

external_components:
  - source: 
      type: git
      url: https://github.com/kumy/esphome_samsung_ac
    components: [samsung_ac]

uart:
  id: uart_bus
  tx_pin: 3
  rx_pin: 2
  baud_rate: 9600

climate:
  - platform: samsung_ac
    name: living-room
    id: living_room
    uart_id: uart_bus
    outdoor_temp:
      name: Outdoor Temp
    runtime_hours_sensor:
      name: Runtime hours
    filtertime_hours_sensor:
      name: Filtertime hours
    used_power_sensor:
      name: Used power
    reset_used_power_button:
      name: Reset used power
    co_mode:
      name: "Co mode"
      modes:
        - "Off"
        - "TurboMode"
        - "Smart"
        - "Sleep" 
        - "Quiet" 
        - "SoftCool"
        - "WindMode1" 
        - "WindMode2"
        - "WindMode3"

[...]
```

The component can be installed locally by downloading to `components` directory or directly from Github.

When configured correctly, new ESPHome device will appear in Home Assistant integrations and you'll be asked to provide encryption key (it's in the node configuration from step 2.). All entities then populate automatically.

![HomeAssistant ESPHome entity](/images/HA_entity.png)

You can then create a (versatile) Thermostat card on the dashboard.

![HomeAssistant card](/images/HA_thermostat.png)


## Links

https://www.espressif.com/en/products/devkits/esp32-devkitc/

Taken this project as a base:
https://github.com/pedobry/esphome_toshiba_suzumi


https://github.com/kumy/samsung-ac
https://github.com/kumy/samsung-ac-simulator


# Run the tests

First launch the dev environment using docker:

```shell
./docker/dev.sh
```

Then from the container you can run the tests:

```shell
./test/run.sh
```