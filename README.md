# nrf24le1-espnrf_gateway_and_remote_switch
This code allows to use an ESP8266 and NRF24LE1 connected each other via rs232 as  gateway to control NRF client by MQTT server.
Connection on gateway side: ESP tx <-> NRF(32pin) p0.4(rx), ESP rx <-> NRF(32pin) p0.3(tx), ESP gpio02 <-> NRF reset.
