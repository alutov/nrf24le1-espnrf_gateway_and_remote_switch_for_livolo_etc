
&emsp; **NRFSRVX.A51** - исходник для NRF24LE1 шлюза. Включаемый файл **EQUNRF24.INC**. Я использовал [Telemark Assembler for Windows XP](http://old-dos.ru/index.php?page=files&mode=files&do=show&id=1385) для 8051 и [документацию](http://www.cpcalive.com/docs/TASMMAN.HTM) для компиляции.  Пример строки компиляции бинарного файла: tasm.exe -51 -b -fff NRFSRVX.A51.</br>
&emsp; **NRFCL0X.A51** - исходник для NRF24LE1 клиента(удаленного выключателя). Включаемый файл **EQUNRF24.INC**. Пример строки компиляции бинарного файла: tasm.exe -51 -b -fff NRFCL0X.A51.</br>
