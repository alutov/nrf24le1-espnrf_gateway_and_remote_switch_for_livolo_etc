# nrf24le1-espnrf_gateway_and_remote_switch

  Gateway code allows to use an ESP8266 and NRF24LE1(espnrf) connected each other via rs232 as a gateway to control NRF24LE1 client
by MQTT server. Connection on gateway side: ESP gpio1(tx) <-> NRF(32pin) p0.4(rx), ESP gpio3(rx) <-> NRF(32pin) p0.3(tx),
ESP gpio02 <-> NRF reset.
  For ESP8266 I use modified code from seb821/espRFLinkMQTT (https://github.com/seb821/espRFLinkMQTT). Thanx for it!)
  The gateway polls 16 NRF clients (up to ~1s for one nrf so full polling time ~16s), with 4 numeric parameters each. The name of
each parameter up to 32 characters must be defined when configuring espnrf  in the web interface.
  Client code allows to use NRF24LE1 as a remote two-channel switch with state monitoring and control of both the impulse and the
livolo or rcswitch protocol. The first 2 parameters reflect the status of the switches (1-on, 0-off), the third parameter is used to
display the temperature from the DS18B20 sensor (relevant for embedding the NRF24LE1 in the air conditioner), the fourth parameter is
the supply voltage of the NRF24LE1. Client switch on RF receiver for ~50mS two times per second then go sleep mode.
  A software update is also supported for both esp and gateway and client nrf. To update the client, it is only necessary that at
least one switch be turned on. (In programming mode, the MCU operates at 16MHz without sleep mode and consumes more.)
  Using the assembler without any sdk allowed the client to reduce the clock frequency by 16 times (1MHz), and when generating the
RC code MCU works at 125 kHz, and accordingly to reduce power consumption, and in the gateway it is free to store in memory not only
the program code, but and reserve space for code updates and for 64 parameter names. I'm still testing the code. My clients control
the air conditioner mitsubishi by “pressing” it on the on-off button, and 2 switches, livolo and maifom. Connection nrf to switches
made by 5 wires: nrf gnd <-> RF gnd, nrf vdd <-> RF +3v,  nrf p0.5(control) <-> RF DATA, p0.0(state1) <-> PIC 11 livolo(LED) or 11 maifom(control1), p0.1(state2) <-> PIC 5 livolo(LED) or 13 maifom(control2). I haven’t done anything else in livolo yet, in maifom
I increased the capacity before ams1117. More in description.doc, a bit later.

