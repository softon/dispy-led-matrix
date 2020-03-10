# Dispy Notice Board

www.dispy.in

## Usage

- Clone the Repo
- Copy the config.h.sample to config.h
- Edit the File with appropriate details
- connect your ESP8266 Module and program
- OTA is enabled so after the first program wireless programming can be done.
- create an account in dispy.in to get the device id.

## Connections
```
//################# DISPLAY CONNECTIONS ################
// LED Matrix Pin -> ESP8266 Pin
// Vcc            -> 3v  (3V on NodeMCU 3V3 on WEMOS)
// Gnd            -> Gnd (G on NodeMCU)
// DIN            -> D7  (Same Pin for WEMOS)
// CS             -> D4  (Same Pin for WEMOS)
// CLK            -> D5  (Same Pin for WEMOS)
```