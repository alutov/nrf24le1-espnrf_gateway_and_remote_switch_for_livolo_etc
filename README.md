# nrf24le1-espnrf_gateway_and_remote_switch

  Gateway code allows to use an ESP8266 and NRF24LE1(espnrf) connected each other via rs232 as a gateway to control NRF24LE1 client
by MQTT server. Connection on gateway side: ESP gpio1(tx) <-> NRF(32pin) p0.4(rx), ESP gpio3(rx) <-> NRF(32pin) p0.3(tx),
ESP gpio02 <-> NRF reset.
  The gateway polls 16 NRF clients, with 4 numeric parameters each. The name of each parameter up to 32 characters must be defined
when configuring in the espnrf web interface.
  Client code allows to use NRF24LE1 as a remote two-channel switch with state monitoring and control of both the impulse and the
livolo or rcswitch protocol. The first 2 parameters reflect the status of the switches (1-on, 0-off), the third parameter is used to
display the temperature from the DS18B20 sensor (relevant for embedding the NRF24LE1 in the air conditioner), the fourth parameter is
the supply voltage of the NRF24LE1. 
  A software update is also supported for both esp and gateway and client nrf. To update the client, it is only necessary that at
least one switch be turned on. (In programming mode, the MCU operates at 16MHz without sleep mode and consumes more.)
  Using the assembler without any sdk allowed the client to reduce the clock frequency by 16 times (1MHz), and when generating the
RC code MCU works at 125 kHz, and accordingly to reduce power consumption, and in the gateway it is free to store in memory not only
the program code, but and reserve space for code updates and for 64 parameter names. I'm still testing the code. My clients control
the air conditioner mitsubishi by “pressing” it on the on-off button, and 2 switches, livolo and maifom. I haven’t done anything in
livolo yet, in maifom I increased the capacity before ams1117. More in description.doc, a bit later.

  Gateway код позволяет шлюзу SP8266 и NRF24LE1(espnrf), соединенными по rs232, управлять NRF24LE1 клиентом  при помощи MQTT сервера.
Соединения на стороне шлюза ESP gpio1(tx) <-> NRF(32pin) p0.4(rx), ESP gpio3(rx) <-> NRF(32pin) p0.3(tx), ESP gpio02 <-> NRF reset.
  Шлюз опрашивает 16 NRF клиентов, по 4 числовых параметра в каждом. Имя каждого параметра до 32 символов должно быть определено
при конфигурации в espnrf web интерфейсе.  
  Client код позволяет использовать NRF24LE1 как удаленный беспроводной двухканальный выключатель с контролем состояния и управлением
как импульсом, так и по протоколу livolo или rcswitch. Первые 2 параметра отражают состояния выключателей (1-включен, 0-выключен),
третий параметр используется для вывода температуры с датчика DS18B20 (актуально для встраивания NRF24LE1 в кондиционер), четвертый
параметр - напряжение питания NRF24LE1. 
  Поддерживается также обновление программы как для esp, так и nrf шлюза и клиента. Для обновления клиента необходимо только, чтобы
хотя бы один выключатель был включен. (В режиме программирования MCU работает на 16MHz без режима сна и потребляет больше.)
  Использование ассемблера без всяких sdk позволило в клиенте понизить тактовую в частоту в 16 раз(1MHz), а при генерации RC кода
MCU работает на 125 kHz, и соответственно снизить потребление энергии, а в шлюзе свободно вместить в память не только код программы,
но и зарезервировать место для обновления кода и для 64 имен параметров. Код еще тестирую. У меня клиенты управляют кондиционером
mitsubishi, "нажимая" ему на кнопку вкл-выкл, и 2-мя выключателями, livolo и maifom.  В livolo пока ничего не делал, в maifom
увеличил емкость перед ams1117. Подробнее в description.doc, чуть позже. 
