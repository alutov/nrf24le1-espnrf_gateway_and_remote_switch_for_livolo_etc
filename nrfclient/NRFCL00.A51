;******************************************************;
;   NRF24LE1 2 switch control program                  ;
;   Lutov Andrey, Donetsk                              ;
;******************************************************;
;               For quartz 16 MHz                      ;
;******************************************************;
;                addresses map:                        ;
;         ----------------------------                 ;
; 00000-017FFh - running firmware   (6144bytes)        ;
; 01800-027FFh - store new firmware via RF (6144bytes) ;
; 03000-03F7Fh - program flash not used                ;
; 03F80-03FFFh - flash program proc                    ;
; 00000-001FFh - retentive XRAM  (RF RX TX buffers)    ;
; 00200-003FFh - non retentive XRAM (not used)         ;
; 0FA00-0FBFFh - extended endurance NVRAM (not used)   ;
; 0FC00-0FFFFh - NVRAM (not used)                      ;
;******************************************************;
; Firmware works in several time slots:                ;
; 1. RF slot (~50mS) - receive&process&transmit frame  ;
;    MCU runs @1MHz, Ints enabled                      ;
; 2. Sleep (~500mS) or send RC code (~500mS) instead   ;
;    When send RC code MCU runs @125kHz, Ints disabled ;
;    DS18B20 readed before NRF enters sleep mode       ;
;******************************************************;
; RF frame 24 bytes:                                   ;
; --------------------------------------------         ;
; |00  |01   |02 03  |04 05 06 07|08 09 0A 0B|         ;
; |----|-----|-------|-----------|-----------|         ;
; |FRID|FLAGS|ADDR   |data1 LSB  |data2 LSB  |         ;
; --------------------------------------------         ;
; ---------------------------------------              ;
; |0C 0D 0E 0F|10 11 12 13| 14 15| 16 17|              ;
; |-----------|-----------|------|------|              ;
; |data3 LSB  |data4 LSB  | CRC  |Unused|              ;
; ---------------------------------------              ;
; ADDR, CRC used in load firmware frame only           ;
; flag bits 3-0 set - set data 4-1                     ;
; flag bits 3-0 reset - read data 4-1                  ;
; data1&2- switch 1&2 states, data3 - temp(r/o),       ;
; data4 - vdd(r/o)                                     ;
;******************************************************;
; P0.0 - sw1 in, P0.1 - sw2 in, P0.2 - sw1 pulse       ;
; P0.3 - sw2 pulse, P0.4 - ds18b20, P0.5 - rc433 out   ;
;******************************************************;

.nolist
.include equnrf24.inc
.list
; nrfid
RFWID     .equ  1H               ;device ID
RRFID     .equ  0H               ;device ADDR
ARFID     .equ  RRFID+0E0H      
;RF values
RFNRETR   .equ 0F0H              ;max_rf_retr_count*2
RFRXRWP   .equ 018H              ;number of payload bytes in RF frame
RFCH      .equ 064H              ;RF channel
; rc433
RCPRT     .equ 000H              ;rc protocol 0-livolo, <>0 - rcswitch
; rc433 code for sw1 livolo
RCSL11    .equ 065H
RCSL12    .equ 0C2H
RCSL13    .equ 000H
; rc433 code for sw2 livolo
RCSL21    .equ 065H
RCSL22    .equ 0C4H
RCSL23    .equ 000H
; rc433 code for sw1 rcswitch
RCSW11    .equ 0E7H
RCSW12    .equ 00BH
RCSW13    .equ 012H
; rc433 code for sw2 rcswitch
RCSW21    .equ 0E7H
RCSW22    .equ 00BH
RCSW23    .equ 018H
;for 1MHz
HI_TIMER  .equ 05DH              ;2Hz timer Int frequency high divisor
LO_TIMER  .equ 03CH              ;low(1M/12/2=41667=A2C3. FFFF-A2C3=5D3C)
;RF timers data
HI_T8mSF  .equ 0D6H              ;8MS @16MHz
LO_T8mSF  .equ 054H
HI_T8mS   .equ 0FDH              ;8MS @1MHz
LO_T8mS   .equ 064H
HI_T480uS .equ 0FFH              ;480uS @1MHz 30uS @16MHz
LO_T480uS .equ 0D7H
HI_T60uS  .equ 0FFH              ;60uS @1MHz
LO_T60uS  .equ 0FCH
HI_T50mS  .equ 0EFH              ;50MS @1MHz
LO_T50mS  .equ 0B8H
HI_T50mSF .equ 0H                ;50MS @16MHz
LO_T50mSF .equ 0H  

;XRAM
;RFRXBUF   .equ 000H              ;24 bytes rf rx buf
RFRXBUFH  .equ 00H               ;24 bytes rf rx buf  (32 reserved)
RFRXBUFL  .equ 00H               ;24 bytes rf rx buf  (32 reserved)
;RFTXBUF   .equ 020H              ;24 bytes rf tx buf
RFTXBUFH  .equ 00H               ;24 bytes rf tx buf  (32 reserved)
RFTXBUFL  .equ 20H               ;24 bytes rf tx buf  (32 reserved)

;program ROM values
NRFWOFS   .equ 18H               ;high address to load fw in program memory
NAMTABH   .equ 30H               ;high address of names tab in program memory

;internal RAM
;Bits
F_SLEEP   .equ B20.0             ;enable goto sleep
F_RFREC   .equ B20.1             ;received frame in buffer
F_RFSND   .equ B20.2             ;transmit frame in buffer
F_LDFW    .equ B20.3             ;loading fw flag
F_DSRD    .equ B20.4             ;ds data ready

;internal RAM      
;Bytes
RFID      .equ 30H               ;current rfid
FWADRL    .equ 31H               ;loading fw address low
FWADRH    .equ 32H               ;loading fw address high
FWWT      .equ 33H               ;wait for flashing
ADCN      .equ 34H               ;vdd adc 
TEMPL     .equ 35H               ;temp value low
TEMPH     .equ 36H               ;temp value high
TEMPC     .equ 37H               ;temp read frequency counter
SW1RSC    .equ 38H               ;switch rc433 code send
SW2RSC    .equ 39H               ;counters
SW1RQS    .equ 3AH               ;sw1 state request
SW2RQS    .equ 3BH               ;sw2 state request
SW1PWT    .equ 3CH               ;wait for out pulse for sw1
SW2PWT    .equ 3DH               ;wait for out pulse for sw2

;code
MAIN:
	.org 0h
	NOP
        AJMP  MAIN1
	.org 3h
	RETI
	.org 06h
	.DB RFWID                ;FW id
CRFID:	.DB RRFID                ;RF id
	.org 0bh
        AJMP  TI0INT
	.org 13h
        RETI
	.org 1bh
        AJMP  TI1INT
	.org 23h
        RETI
	.org 2Bh
        RETI
	.org 43h
        RETI
	.org 4Bh
        AJMP  RFINT
	.org 53h
        RETI
	.org 5Bh
        RETI
	.org 63h
        RETI
	.org 6Bh
	ANL   RTC2CON,#0FEH
        RETI

	.DB 13,10
	.DB 13,10
;******************************************************;
	.text NRF24_switch_
VERNRF	.TEXT v0.00_21.07.19
;******************************************************;
	.DB 0
	.DB 13,10
	.DB 13,10

;******************************************************;
TI1INT:                 ;Timer interrupt 1 routine
;******************************************************;
        PUSH  PSW                ;save contents
        PUSH  ACC                ;
        CLR   TCON.6             ;reload  timer1
        MOV   PSW,#8H            ;select bank 1 (08H-0FH)
        MOV   TH1,#HI_TIMER      ;timer1- int 2 Hz
        MOV   TL1,#LO_TIMER      ;
        SETB  TCON.6             ;restart timer1
        MOV   R0,#7FH            ;Yes, get 1 byte on stack top
        MOV   A,@R0              ;
        XRL   A,#11H             ;stack overflow?
        JZ    TI1INT2            ;no
TI1INT1:
        POP   ACC                ;yes, halt
        POP   PSW                ;restart
	CLR   IEN0.7
        LJMP  MAIN
TI1INT2:
        DEC   R0
        MOV   A,@R0              ;get 2 byte on stack top
        JNZ   TI1INT1            ;currently timer1 stack supervision only

TI1INTE:
        POP   ACC                ;restore registers
        POP   PSW                ;and return
        RETI                     ;

;******************************************************;
TI0INT:                 ;Timer interrupt 0 routine
;******************************************************;
        PUSH  PSW                ;save contents
        PUSH  ACC
	PUSH  MPAGE
        MOV   PSW,#18H           ;select bank 3 (18H-1FH) 
	CLR   TCON.4
	CLR   F_SLEEP
	MOV   A,R7
	JNZ   TI0INT1            ;if RF inactive
	MOV   MPAGE,#RFTXBUFH
	MOV   R0,#RFTXBUFL
	MOV   A,#0FFH
	MOVX  @R0,A              ;set invalid TX frid
        CLR   RFCON.1            ;0-set RX mode
	MOV   A,#20H
	LCALL BSNSPI
	MOV   A,#0FH
	LCALL BSNSPI
	SETB  RFCON.1
	SETB  RFCON.0
	CLR   F_RFREC
	INC   R7
        MOV   TH0,#HI_T8mS
        MOV   TL0,#LO_T8mS
	MOV   A,CLKCTRL
	JNZ   TI0INT6
        MOV   TH0,#HI_T8mSF
        MOV   TL0,#LO_T8mSF
	SJMP  TI0INT6
TI0INT1:CJNE  A,#RFNRETR,$+3
	JNC   TI0INT2            ;if max retransmission
        JNB   ACC.0,TI0INT5
        JB    F_RFREC,TI0INT4    ;frame received? 
TI0INT2:CLR   RFCON.0            ;inactive rf
	CLR   F_RFREC
	MOV   R7,#00H
	JB    F_LDFW,TI0INT3
	SETB  F_SLEEP
TI0INT3:MOV   TH0,#HI_T50mS      ;HI_T8mS       ;HI_T50mS
        MOV   TL0,#LO_T50mS      ;LO_T8mS       ;LO_T50mS
	MOV   A,CLKCTRL
	JNZ   TI0INT6
        MOV   TH0,#HI_T50mSF     ;HI_T8mSF        ;HI_T50mSF
        MOV   TL0,#LO_T50mSF     ;LO_T8mSF        ;LO_T50mSF
	SJMP  TI0INT6
TI0INT4:CLR   RFCON.0            ;acc.0=0-set TX mode
        CLR   RFCON.1
	MOV   A,#20H
	LCALL BSNSPI
	MOV   A,#0EH
	LCALL BSNSPI
	SETB  RFCON.1
	INC   R7
        MOV   TH0,#HI_T60uS
        MOV   TL0,#LO_T60uS
	MOV   A,CLKCTRL
	JNZ   TI0INT6
        MOV   TH0,#HI_T480uS
        MOV   TL0,#LO_T480uS
	SJMP  TI0INT6
TI0INT5:JB    F_RFSND,TI0INT7
	MOV   A,R6
	INC   A
	CJNE  A,#010H,$+3
	JNC   TI0INT2
	MOV   R6,A
        MOV   TH0,#HI_T480uS
        MOV   TL0,#LO_T480uS
TI0INT6:SETB  TCON.4             ;start timer0
	SETB  IEN0.1             ;en timer0
	SJMP  TI0INTE
TI0INT7:MOV   MPAGE,#RFTXBUFH
	MOV   R0,#RFTXBUFL	 ;TX buffer
	MOV   R1,#RFRXRWP        ;TX count
	CLR   RFCON.1
	MOV   A,#0A0H            ;send
	LCALL BSNSPI
TI0INT8:MOVX  A,@R0
	LCALL BSNSPI
	MOV   A,R0
	INC   A
	MOV   R0,A
	JNZ   TI0INT9
	INC   MPAGE
TI0INT9:DJNZ  R1,TI0INT8	
	SETB  RFCON.1
	SETB  RFCON.0            ;activate TX
        CLR   TCON.4             ;stop timer0
	CLR   IEN0.1             ;dis timer0
	INC   R7
TI0INTE:POP   MPAGE
        POP   ACC                ;restore registers
        POP   PSW                ;and return
        RETI

;******************************************************;
RFINT:                 ;RF interrupt routine
;******************************************************;
        PUSH  PSW                ;save used registers
        PUSH  ACC                ;
	PUSH  MPAGE
        MOV   PSW,#18H           ;select bank 3 (18H-1FH) 
RFINT1: CLR   RFCON.1            ;read STAT
	MOV   A,#07H	
	LCALL BSNSPI
	MOV   R2,A
	CLR   A
	LCALL BSNSPI
	SETB  RFCON.1
	MOV   A,R2               ;RX int?
	JNB   ACC.6,RFINT6       ;yes
	CLR   F_RFREC
	MOV   MPAGE,#RFRXBUFH
	MOV   R0,#RFRXBUFL
	MOV   R1,#RFRXRWP        ;31+1=32 bytes
	DEC   R1
	CLR   RFCON.1
	MOV   A,#061H
	LCALL BSNSPI
	CLR   A
	LCALL BSNSPI
	MOVX  @R0,A
	ANL   A,#7FH             ;ignore 7 bit
	MOV   R2,A               ;RX_ADDR
RFINT2: MOV   A,R0
	INC   A
	MOV   R0,A
	JNZ   RFINT3
	INC   MPAGE
RFINT3: CLR   A
	LCALL BSNSPI
	MOVX  @R0,A
	DJNZ  R1,RFINT2	
	SETB  RFCON.1
	MOV   A,R2
	CJNE  A,RFID,RFINT5
	SETB  F_RFREC
	MOV   R6,#00H
	CLR   RFCON.0            ;if no set RX inactive
        CLR   RFCON.1            ;set TX mode
	MOV   A,#20H
	LCALL BSNSPI
	MOV   A,#0EH
	LCALL BSNSPI
	SETB  RFCON.1
	CLR   TCON.4             ;stop TF0
        MOV   TH0,#HI_T60uS
        MOV   TL0,#LO_T60uS
	MOV   A,CLKCTRL
	JNZ   RFINT4
        MOV   TH0,#HI_T480uS
        MOV   TL0,#LO_T480uS
RFINT4: SETB  TCON.4             ;start timer0
	SETB  IEN0.1             ;en timer0
RFINT5: MOV   A,#40H             ;clear RX_DR
	SJMP  RFINT8
RFINT6: MOV   C,ACC.4            ;TX int? MAX_RT
	ORL   C,ACC.5            ;TX_DS
	JNC   RFINTE
	CLR   F_RFREC
	CLR   F_RFSND
        CLR   RFCON.1            ;0-set RX mode
	MOV   A,#20H
	LCALL BSNSPI
	MOV   A,#0FH
	LCALL BSNSPI
	SETB  RFCON.1
	CLR   TCON.4
        MOV   TH0,#HI_T8mS
        MOV   TL0,#LO_T8mS
	MOV   A,CLKCTRL
	JNZ   RFINT7
        MOV   TH0,#HI_T8mSF
        MOV   TL0,#LO_T8mSF
RFINT7: SETB  TCON.4             ;restart timer0
	SETB  IEN0.1             ;en timer0
	MOV   A,#30H             ;clear TX_DS+MAX_RT
RFINT8: MOV   R2,A
        CLR   RFCON.1
	MOV   A,#27H	
	LCALL BSNSPI
	MOV   A,R2
	LCALL BSNSPI
	SETB  RFCON.1
RFINTE: POP   MPAGE              ;restore used registers
        POP   ACC
        POP   PSW
        RETI

;******************************************************;
; BSNSPI write_read byte in A to from SPI
;******************************************************;
BSNSPI: MOV   SPIRDAT,A
BSNSPI1:MOV   A,SPIRSTAT
	JNB   ACC.1,BSNSPI1
	CLR   IRCON.0
	MOV   A,SPIRDAT
	RET

;******************************************************;
;RFINIT NRF24 initialize
;******************************************************;
RFINIT:
	MOV   RFCON,#00H         ;all off
;wait
	LCALL TWAIT
	SETB  RFCON.2            ;Clock Enable
;wait	
	LCALL TWAIT
	MOV   DPTR,#INITRFDAT
RFINIT1:
	CLR   A
	MOVC  A,@A+DPTR
	INC   DPTR
	MOV   R1,A
	JZ    RFINIT5
	CLR   RFCON.1
RFINIT3:
	CLR   A
	MOVC  A,@A+DPTR
	INC   DPTR
	LCALL BSNSPI
	DJNZ  R1,RFINIT3
	SETB  RFCON.1
	SJMP  RFINIT1
RFINIT5:
	RET

;******************************************************;
;little wait for rfinit proc only
;******************************************************;
TWAIT: 	MOV   R1,#060H
TWAIT1: NOP
 	DJNZ  R1,TWAIT1
	RET			


;******************************************************;
MAIN1:                          ;Start after RESET
;******************************************************;
;	MOV   CLKCTRL,#00H       ;set clock 16Mhz
	MOV   CLKCTRL,#04H       ;set clock 1Mhz
        MOV   IEN0,#0
        MOV   IEN1,#0
	MOV   INTEXP,#00H
        MOV   PSW,#0             ;select bank 0
	MOV   MEMCON,#00H        ;XRAM for data from 0
        MOV   MPAGE,#00H
	MOV   DPS,#00H
        MOV   A,#11H             ;stack overflow control
        MOV   R0,#7FH
MAIN2:                           ;clear internal RAM
        MOV   @R0,A
        CLR   A
        DJNZ  R0,MAIN2
	MOV   @R0,A
        MOV   SP,#60H            ;set stack
;ports
;for positive pulse
	CLR   P0.5
;for negative pulse
;	SETB  P0.5
;
	MOV   P0DIR,#0DFH        ;P0.5 out
	MOV   P0CON,#30H         ;P0.0 - input pull down
	MOV   P0CON,#31H         ;P0.1 - input pull down
;for positive pulse
;	MOV   P0CON,#32H         ;P0.2 - input pull down
;	MOV   P0CON,#33H         ;P0.3 - input pull down
;for negative pulse
	MOV   P0CON,#52H         ;P0.2 - input pull up
	MOV   P0CON,#53H         ;P0.3 - input pull up
;
	MOV   P0CON,#54H         ;P0.4 - input pull up
	MOV   P0CON,#56H         ;P0.6 - input pull up
;
	MOV   TEMPL,#0FFH        ;invalid temp -0.0
	MOV   TEMPH,#0FFH
	ACALL READDS
;clear xram
	MOV   DPTR,#0
MAIN3:
        CLR   A
	MOVX  @DPTR,A
	INC   DPTR
	MOV   A,DPH
	CJNE  A,#04H,MAIN3
;
	MOV   DPTR,#CRFID        ;copy rfid from rom to ram
	CLR   A
	MOVC  A,@A+DPTR
        MOV   RFID,A
;uart (not used)
	MOV   ADCON,#80H         ;additional internal baud rate generator used
        MOV   PCON,#80H          ;set SMOD bit for UART
	MOV   S0RELL,#0F3H       ;3F3H-38400(16),19200(8),9600(4),4800(2), 2400(1)
	MOV   S0RELH,#03H
	MOV   S0CON,#70H         ;8bit
;ints priority
        MOV   IP0,#17H           ;priority >> TF0,RF, SPI, UART2, TF1
        MOV   IP1,#00H           ;
;timers
        MOV   TMOD,#11H          ;timers mode 1
        MOV   TH1,#HI_TIMER      ;timer1 - int 16 Hz
        MOV   TL1,#LO_TIMER
	MOV   TCON,#40H          ;stop timer 0, start timer1
;adc
	MOV   ADCCON1,#38H       ;1.2V ref +1/3VDD
	MOV   ADCCON2,#00H       ;
	MOV   ADCCON3,#040H      ;8 bit
;power 
        MOV   POFCON,#000H       ;
;rtc2
	MOV   CLKLFCTRL,#1
	MOV   RTC2CMP0,#000H     ;0.5s sleep time
	MOV   RTC2CMP1,#040H
	MOV   RTC2CON,#06H       ;not reset #06H
	ANL   RTC2CON,#0FEH
;wakeup
	MOV   WUOPC0,#00H        ;no pin wakeup
	MOV   WUOPC1,#00H        ;no pin wakeup
	MOV   OPMCON,#04H        ;active low 
;timer 0
        MOV   TH0,#0
        MOV   TL0,#0
        SETB  TCON.4             ;start timer0
	SETB  IEN0.1             ;en timer0
;ints
	SETB  IEN0.7             ;en all
	SETB  IEN0.3             ;en timer1
	SETB  IEN1.5             ;rtc2 TICK
	SETB  IEN1.1             ;RFIRQ
;rf
MAIN5:
        LCALL RFINIT             ;RF initialisation
;main loop code
MAIN6:
	ACALL RFREAD             ;read & parse rf frames
	JNB   F_SLEEP,MAIN6      ;ready for sleep?
	CLR   TCON.4             ;yes
	CLR   TCON.6             ;stop timers
	ACALL PSWPL              ;check sw1&sw2 pulse completed
	MOV   B,#00H             ;sw1
	MOV   A,SW1RSC           ;send code request for sw1?
	JNZ   MAIN8              ;yes
	MOV   B,#01H             ;sw2
	MOV   A,SW2RSC           ;send code request for sw2?
	JNZ   MAIN8              ;yes
	MOV   A,TEMPC            ;before sleep
	JNZ   MAIN7              ;every 16 sleep
	ACALL READDS             ;read ds18b20
MAIN7:  INC   TEMPC              ;next
	ANL   TEMPC,#1FH         ;16
	ORL   RTC2CON,#1         ;start rtc2
	MOV   PWRDWN, #04H       ;sleep
	ANL   RTC2CON,#0FEH      ;stop rtc2
	SJMP  MAIN9
MAIN8:  ACALL PRCSW              ;send rf code via p0.5
MAIN9:  
	CLR   F_SLEEP            ;clear sleep flag
	SETB  TCON.4             ;start timers
	SETB  TCON.6
	SJMP  MAIN5


RFREAD: PUSH  MPAGE              ;read & parse rf frames
        JB    F_RFSND,RFREAD1    ;exit if data sending
        JB    F_RFREC,RFREAD3    ;exit if no received frame
RFREAD1:AJMP  RFREADE
RFREAD2:AJMP  RFREAD22
RFREAD3:MOV   MPAGE,#RFTXBUFH    ;mpage:r0 - tx buf
	MOV   R0,#RFTXBUFL
	MOV   DPH,#RFRXBUFH      ;dptr - rx buf
	MOV   DPL,#RFRXBUFL
	MOVX  A,@DPTR            ;read rfid
	MOVX  @R0,A              ;store in tx
	INC   R0
	INC   DPTR	
	JB    ACC.7,RFREAD2      ;goto if ldfw
	CJNE  A,RFID,RFREAD1     ;exit if rfid not match 
;send data part
	ORL   ADCCON1,#80H       ;start ADC conversion
	CLR   F_LDFW             ;cancel fw load
	MOV   A,R0               ;store in r1 pointer to flags
	MOV   R1,A               ;currently not used
	MOVX  A,@DPTR            ;read flags in r2
	MOV   R2,A
	INC   DPTR
	MOVX  @R0,A              ;store rx flags to tx
	INC   R0
	MOV   R3,#02H
RFREAD4:MOVX  A,@DPTR            ;copy to tx 2 addr bytes 
	MOVX  @R0,A	
	INC   DPTR
	INC   R0
	DJNZ  R3,RFREAD4
	MOVX  A,@DPTR            ;receive state to r3
	JZ    RFREAD5            ;if <>0 
	MOV   R3,#01H            ;then =1
RFREAD5:CLR   A
	MOV   C,P0.0             
	MOV   ACC.0,C
	MOV   R4,A               ;read current state to r4
	XRL   A,R3
	JZ    RFREAD6            ;skip if equal
	MOV   A,R2               
	JNB   ACC.0,RFREAD6      ;if set command
;
;for positive pulse
;	SETB  P0.2               ;p0.2 out
;for negative pulse
	CLR   P0.2
;
	ANL   P0DIR,#0FBH        ;change state to out
	MOV   SW1PWT,#08H        ;wait about 2s
	MOV   A,R3               ;new state
	MOV   SW1RQS,A
	MOV   R4,A               ;to current
	MOV   SW1RSC,#08H
RFREAD6:MOV   R3,#04H
	MOV   A,R4               ;store state 1
RFREAD8:MOVX  @R0,A	
	INC   R0
	INC   DPTR
	CLR   A
	DJNZ  R3,RFREAD8	
	MOVX  A,@DPTR
	JZ    RFREAD9            ;receive state to r3
	MOV   R3,#01H            ;if <>0
RFREAD9:CLR   A                  ;then =1
	MOV   C,P0.1
	MOV   ACC.0,C
	MOV   R4,A               ;read current state to r4
	XRL   A,R3
	JZ    RFREAD10           ;skip if equal
	MOV   A,R2
	JNB   ACC.1,RFREAD10     ;if set command
;
;for positive pulse
;     	SETB  P0.3               ;p0.3 out
;for negative pulse
	CLR   P0.3
;
	ANL   P0DIR,#0F7H        ;change state to out
	MOV   SW2PWT,#08H        ;wait about 2s
	MOV   A,R3               ;new state
	MOV   SW2RQS,A
	MOV   R4,A               ;to current
	MOV   SW2RSC,#08H
RFREAD10:
	MOV   R3,#04H
	MOV   A,R4               ;store state 2
RFREAD12:
	MOVX  @R0,A	
	INC   R0
	INC   DPTR
	CLR   A
	DJNZ  R3,RFREAD12	
	MOV   R2,#10H            ;temp, 1 digit after .
	MOV   A,TEMPL
	MOV   R4,A               ;t=(readed)*10/16
	MOV   A,TEMPH
	MOV   R3,A	
	ANL   A,#0F8H            ;negative?
	JZ    RFREAD13
	MOV   R2,#90H            ;add '-' if t  negative 
	MOV   A,R4
	CPL   A                  ;invert
	ADD   A,#01H             ;& + 1
	MOV   R4,A
	MOV   A,R3
	CPL   A
	ADDC  A,#00H	
	ANL   A,#07H
	MOV   R3,A
RFREAD13:
	MOV   A,R4
	MOV   B,#0AH              ;*10
	MUL   AB
	SWAP  A
	ANL   A,#0FH
	MOV   R4,A
	MOV   A,B
	XCH   A,R3	
	MOV   B,#0AH
	MUL   AB                  ;*10
	ADD   A,R3
	SWAP  A
	MOV   R3,A
	ANL   A,#0F0H
	ORL   A,R4
	MOVX  @R0,A
	INC   R0
	INC   DPTR
	MOV   A,R3
	ANL   A,#0FH
	MOVX  @R0,A
	INC   R0
	INC   DPTR
	CLR   A
	MOVX  @R0,A
	INC   R0
	INC   DPTR
	MOV   A,R2
	MOVX  @R0,A	
	INC   R0
	INC   DPTR
	MOV   A,ADCDATH          ;vdd
	MOV   ADCN,A             ;vddref=1.2v & readded=1/3*meas
	ANL   ADCCON1,#7FH
	MOV   B,ADCN             ;vdd=(readed)*360/256
	MOV   A,#68H             ;360=168h
	MUL   AB
	RLC   A                  ;/256 but acc.7 -> c
	MOV   A,ADCN             ;*1
	ADDC  A,B                ;+ c
	MOVX  @R0,A              ;lsb
	CLR   A
	ADDC  A,#00H
	INC   R0
	MOVX  @R0,A              ;msb
	INC   R0
	CLR   A
	MOVX  @R0,A	
	INC   R0
	MOV   A,#20H             ;2 digits after .
	MOVX  @R0,A	
	CLR   A                  ;if data request
	MOV   FWADRL,A           ;cancel fw load
	MOV   FWADRH,A
	SETB  F_RFSND            ;data ready for send
	MOV   A,FWWT             ;check for loaded update
	XRL   A,#5AH
	JNZ   RFREAD19
	ACALL CHKFW              ;check id & crc once more
	JNZ   RFREAD19
	LJMP  PFLASH             ;jmp to flash proc
RFREAD19:
	CLR   A                  ;reset update flag
	MOV   FWWT,A
	AJMP  RFREADE
;loading code part
RFREAD20:
	MOV   R3,#01H
        AJMP  RFREAD29
RFREAD21:
	MOV   R3,#00H
        AJMP  RFREAD31
RFREAD22:
	MOV   A,R0
	MOV   R7,A
	MOVX  A,@DPTR            ;+1 flags=0
	JNZ   RFREAD20
	INC   DPTR
	MOV   R3,DPH             ;+2 adrl
	MOV   R4,DPL
	MOV   R2,#12H
	LCALL CLCRC              ;calc crc16
	MOVX  A,@DPTR
	INC   DPTR
	XRL   A,R0               ;compare low
	JNZ   RFREAD20           ;error
	MOVX  A,@DPTR
	XRL   A,R1               ;compare high
	JNZ   RFREAD20           ;error
	MOV   C,P0.0             ;for update at least one switch
	ORL   C,P0.1             ;must be on (for enouph power reason)
	JNC   RFREAD20           ;error
	MOV   DPH,R3
	MOV   DPL,R4
	MOVX  A,@DPTR
	MOV   R4,A
	INC   DPTR
	ANL   A,#0FH             ;low address must align 16
	JNZ   RFREAD20
	MOVX  A,@DPTR
	MOV   R3,A
	INC   DPTR               ;+4 DATA
	CJNE  A,#18H,$+3
	JNC   RFREAD20
	MOV   A,R3
	CJNE  A,FWADRH,$+3       ;readed address must equal to expected
	JC    RFREAD21	
	MOV   A,R4
	CJNE  A,FWADRL,$+3
	JC    RFREAD21	
	MOV   A,R3
	ORL   A,R4
	JNZ   RFREAD24
	MOV   A,#NRFWOFS         ;if start ADDR=0
	CLR   C                  ;erase all pages
	RRC   A
	MOV   R0,A	
	MOV   A,#NAMTABH
	CLR   C
	RRC   A
	CLR   C
	SUBB  A,R0
	MOV   R2,A               ;number of pages
	MOV   A,R0
	MOV   R1,CLKCTRL
	CLR   IEN0.7
	MOV   CLKCTRL,#00H       ;erase @ full speed 16MHz
	SETB  FSR.5
RFREAD23:
	MOV   FCR,A              ;erase all pages
	INC   A
	DJNZ  R2,RFREAD23
	CLR   FSR.5
	MOV   CLKCTRL,R1
	SETB  IEN0.7
	CLR   A
	MOV   FWADRL,A
	MOV   FWADRH,A
RFREAD24:
	MOV   A,R3
	ADD   A,#NRFWOFS
	MOV   R3,A
	MOV   DPH1,A
	MOV   A,R4
	MOV   DPL1,A
	MOV   R5,DPH
	MOV   R6,DPL
	MOV   R2,#10H
	MOV   R1,CLKCTRL
	CLR   IEN0.7
	MOV   CLKCTRL,#00H       ;program @ full speed 16MHz
	SETB  FSR.5
RFREAD25:
	MOVX  A,@DPTR
	INC   DPTR
	MOV   DPS,#01H
	ORL   PCON,#10H
	MOVX  @DPTR,A
	ANL   PCON,#0EFH
	INC   DPTR
	MOV   DPS,#00H
	DJNZ  R2,RFREAD25
	CLR   FSR.5
	MOV   CLKCTRL,R1
	SETB  IEN0.7

	MOV   A,R3
	MOV   DPH1,A
	MOV   A,R4
	MOV   DPL1,A
	MOV   R2,#10H            ;verify
	MOV   DPH,R5
	MOV   DPL,R6
	CLR   A
	MOV   R3,A
RFREAD26:
	CLR   A
	MOV   DPS,#01H
	MOVC  A,@A+DPTR					
	MOV   R1,A
	INC   DPTR
	MOV   DPS,#00H
	MOVX  A,@DPTR
	INC   DPTR
	XRL   A,R1
	JZ    RFREAD27
	INC   R3                 ;inc r3 if error
RFREAD27:
	DJNZ  R2,RFREAD26
	MOV   A,R3
	JNZ   RFREAD29
	MOV   A,FWADRL           ;prepare next address
	ADD   A,#10H
	MOV   FWADRL,A
	MOV   A,FWADRH
	ADDC  A,#00H
	MOV   FWADRH,A
	XRL   A,#18H             ;all frames? 
	JNZ   RFREAD30
;
	ACALL CHKFW              ;yes, check fw
	JNZ   RFREAD28
	MOV   FWWT,#5AH          ;set update flag
	SJMP  RFREAD29
RFREAD28:
	MOV   R3,#01H            ;error
	CLR   A                  ;reset address
	MOV   FWADRL,A
	MOV   FWADRH,A
RFREAD29:
	CLR   F_LDFW             ;reset update flag
	SJMP  RFREAD31
RFREAD30:
	SETB  F_LDFW             ;if ok set update flag
RFREAD31:
	MOV   A,R7
	MOV   R0,A
	MOV   A,R3
	MOVX  @R0,A
	INC   R0
	MOV   A,FWADRL
	MOVX  @R0,A
	INC   R0
	MOV   A,FWADRH
	MOVX  @R0,A
	INC   R0
	MOV   R2,#12H
RFREAD32:
	CLR   A
	MOVX  @R0,A
	INC   R0
	DJNZ  R2,RFREAD32
	SETB  F_RFSND            ;data ready for send
RFREADE:POP   MPAGE
	RET

CHKFW:	MOV   DPL,#00H           ;check firmware
	MOV   DPH,#NRFWOFS
	MOV   A,#NAMTABH
	DEC   A
	MOV   R2,A
	MOV   R3,#0FEH
	LCALL CPCRC
	CLR   A
	MOVC  A,@A+DPTR
	INC   DPTR
	XRL   A,R0
	JNZ   CHKFWE
	CLR   A
	MOVC  A,@A+DPTR
	XRL   A,R1
	JNZ   CHKFWE
	MOV   DPTR,#06H
	CLR   A
	MOVC  A,@A+DPTR          ;check 6,7
	MOV   R0,A
	INC   DPTR
	CLR   A
	MOVC  A,@A+DPTR
	MOV   R1,A
	MOV   DPH,#NRFWOFS
	MOV   DPL,#06H
	CLR   A
	MOVC  A,@A+DPTR
	INC   DPTR
	XRL   A,R0
	JNZ   CHKFWE
	CLR   A
	MOVC  A,@A+DPTR
	XRL   A,R1
CHKFWE: RET

PSWPL:  MOV   A,SW1PWT
	JZ    PSWPL2
	DEC   A
	MOV   SW1PWT,A
	JZ    PSWPL1
	CLR   A                  ;for switch 1
	MOV   C,P0.0             ;read state sw1
	MOV   ACC.0,C	
	XRL   A,SW1RQS           ;compare to requested
	JNZ   PSWPL2             ;if equal exit
	MOV   SW1PWT,A
PSWPL1:	ORL   P0DIR,#004H
PSWPL2:	MOV   A,SW2PWT
	JZ    PSWPL4
	DEC   A
	MOV   SW2PWT,A
	JZ    PSWPL3
	CLR   A                  ;for switch 2
	MOV   C,P0.1             ;read state sw2
	MOV   ACC.0,C	
	XRL   A,SW2RQS           ;compare to requested
	JNZ   PSWPL4             ;if equal exit
	MOV   SW2PWT,A
PSWPL3:	ORL   P0DIR,#008H
PSWPL4:	RET


PRCSW:	MOV   A,CLKCTRL          ;switch number in b
	MOV   R7,A               ;store current clock value to r7
	MOV   CLKCTRL,#07H       ;set minimal clock 125kHz
	MOV   A,#RCPRT           ;livolo or rcswitch?
	JNZ   PRCSW16
;	JB    P0.6,PRCSW16       ;livolo or rcswitch?
	MOV   R5,#60H            ;0-livolo, max 96 times to send
PRCSW1:	MOV   A,B                ;max 24*0.3*96=691mS
	JNZ   PRCSW2
	MOV   R0,#RCSL11         ;data for switch 1
	MOV   R1,#RCSL12
	MOV   R2,#RCSL13
	SJMP  PRCSW4             ;for equal timing sw1 & sw2
PRCSW2:	MOV   R0,#RCSL21         ;data for switch 2
	MOV   R1,#RCSL22
	MOV   R2,#RCSL23
	SJMP  PRCSW4             ;for equal timing sw1 & sw2
PRCSW3:	MOV   C,P0.5             ;additional delay if sending 1
	SJMP  PRCSW7             ;~145uS+~145uS+~10uS=~300uS
PRCSW4:	MOV   R4,#17H            ;23(16id+7code) bits to send
;for positive pulse
	SETB  P0.5               ;send start bit high - 500uS
;for negative pulse
;	CLR   P0.5
;
	MOV   R3,#0BH
PRCSW5: NOP
	NOP
	DJNZ  R3,PRCSW5
PRCSW6:	CPL   P0.5               ;loop fo all bits
	MOV   A,R2               ;invert port
	RLC   A                  ;rotate
	MOV   R2,A               ;all
	MOV   A,R1               ;left
	RLC   A	                 ;and
	MOV   R1,A               ;delay
	MOV   A,R0               ;~145uS
	RLC   A                  ;bit to send in c
	MOV   R0,A
	JC    PRCSW3             ;goto delay if 1
	CPL   P0.5               ;if 0 invert port
PRCSW7: MOV   R3,#02H            ;delay
PRCSW8:	NOP                      ;~145uS
	NOP
	DJNZ  R3,PRCSW8
	DJNZ  R4,PRCSW6          ;goto next bit
;for positive pulse
        CLR   P0.5	
;for negative pulse
;	SETB  P0.5
;
	MOV   R3,#04H            ;low delay at the end
PRCSW9:	NOP
	NOP
	DJNZ  R3,PRCSW9          ;whole frame sended
	DJNZ  R5,PRCSW1          ;send all 96 frames because of livolo led blinking in teach mode
	MOV   A,B                ;verify if switch state changed to request
	JNZ   PRCSW13            ;switch number?
	CLR   A                  ;for switch 1
	MOV   C,P0.0             ;read state sw1
	MOV   ACC.0,C	
	XRL   A,SW1RQS           ;compare to requested
	JZ    PRCSW11            ;if equal clear windows counter & exit
	MOV   A,SW1RSC           ;if all 96 frames sended but no switch
	JZ    PRCSW12            ;send in next window
	DEC   A                  ;decrement windows counter
PRCSW11:MOV   SW1RSC,A           ;store&exit
PRCSW12:AJMP  PRCSWE
PRCSW13:CLR   A                  ;for switch 2
	MOV   C,P0.1             ;read state sw2
	MOV   ACC.0,C	
	XRL   A,SW2RQS           ;compare to requested
	JZ    PRCSW14            ;if equal clear windows counter & exit
	MOV   A,SW2RSC           ;if all 96 frames sended but no switch
	JZ    PRCSW15            ;send in next window
	DEC   A                  ;decrement windows counter
PRCSW14:MOV   SW2RSC,A           ;store&exit
PRCSW15:AJMP  PRCSWE
PRCSW16:MOV   R5,#0FH            ;<>0-rcswitch, max 15 times to send
;inactive data line if probably active (for Maifom)
;for positive pulse
        CLR   P0.5	
;for negative pulse
;	SETB  P0.5
;
PRCSW17:MOV   A,B
	JNZ   PRCSW18
	MOV   R0,#RCSW11         ;data for switch 1
	MOV   R1,#RCSW12
	MOV   R2,#RCSW13
	SJMP  PRCSW19            ;for equal timing sw1 & sw2
PRCSW18:MOV   R0,#RCSW21         ;data for switch 2
	MOV   R1,#RCSW22
	MOV   R2,#RCSW23
	SJMP  PRCSW19            ;for equal timing sw1 & sw2
PRCSW19:MOV   R4,#18H            ;24 bits to send
PRCSW20:
;for positive pulse
	SETB  P0.5               ;loop fo all bits
;for negative pulse
;	CLR   P0.5
;
	MOV   A,R2               ;set port
	RLC   A                  ;rotate
	MOV   R2,A               ;all
	MOV   A,R1               ;left
	RLC   A	                 ;and
	MOV   R1,A               ;delay
	MOV   A,R0
	RLC   A                  ;bit to send in c
	MOV   R0,A
	MOV   R3,#16H            ;high delay depend on c
	JC    PRCSW21            ;0 - 350uS, 1 - 3*350uS
	MOV   A,P0               ;delay correction
	MOV   R3,#04H
PRCSW21:NOP
	NOP
	DJNZ  R3,PRCSW21
	CPL   P0.5               ;clear port
	MOV   R3,#07H            ;low delay depend on c
	JC    PRCSW22            ;0 - 3*350uS, 1 - 350uS
	MOV   R3,#18H
PRCSW22:NOP
	NOP
	DJNZ  R3,PRCSW22
	DJNZ  R4,PRCSW20         ;goto next bit
;for positive pulse
	SETB  P0.5               ;sync pulse set port
;for negative pulse
;	CLR   P0.5
;
	MOV   R3,#07H            ;high - 350uS
PRCSW23:NOP
	NOP
	DJNZ  R3,PRCSW23
	CPL   P0.5               ;sync pulse clear port
	MOV   R3,#0D9H           ;low 31*350uS
PRCSW24:NOP
	NOP
	DJNZ  R3,PRCSW24         ;whole frame sended
	MOV   A,B                ;verify if switch state changed to request
	JNZ   PRCSW26            ;switch number?
	CLR   A                  ;for switch 1
	MOV   C,P0.0             ;read state sw1
	MOV   ACC.0,C	
	XRL   A,SW1RQS           ;compare to requested
	JZ    PRCSW25            ;if equal clear windows counter & exit
	DJNZ  R5,PRCSW17         ;if no send frame once more
	MOV   A,SW1RSC           ;if all 15 frames sended but no switch
	JZ    PRCSW28            ;send in next window
	DEC   A                  ;decrement windows counter
PRCSW25:MOV   SW1RSC,A           ;store&exit
	SJMP  PRCSW28
PRCSW26:CLR   A                  ;for switch 2
	MOV   C,P0.1             ;read state sw2
	MOV   ACC.0,C	
	XRL   A,SW2RQS           ;compare to requested
	JZ    PRCSW27            ;if equal clear windows counter & exit
	DJNZ  R5,PRCSW17         ;if no send frame once more
	MOV   A,SW2RSC           ;if all 15 frames sended but no switch
	JZ    PRCSW28            ;send in next window
	DEC   A                  ;decrement windows counter
PRCSW27:MOV   SW2RSC,A           ;store&exit
PRCSW28:
;for Maifom switch leave line active because of noise from RF receiver
;for positive pulse
	SETB  P0.5
;for negative pulse
;	CLR   P0.5
;
PRCSWE:	MOV   A,R7
	MOV   CLKCTRL,A
	RET


READDS: JNB   F_DSRD,READDS1     ;convert t command sended in previous
	ACALL DSRES              ;16th sleep window? if yes reset w1 bus
	JC    READDS1            ;if no device exit
	MOV   A,#0CCH            ;skip rom command
	ACALL DSCOMM
	MOV   A,#0BEH            ;read command
	ACALL DSCOMM
	ACALL DSREAD             ;read 2 bytes only
	MOV   TEMPL,A            ;low temp
	ACALL DSREAD
	MOV   TEMPH,A            ;high temp
READDS1:CLR   F_DSRD             ;clear read flag
	ACALL DSRES              ;reset bus
	JC    READDS2            ;if no device exit
	MOV   A,#0CCH            ;skip rom
	ACALL DSCOMM
	MOV   A,#044H            ;convert t command
	ACALL DSCOMM             ;send
	SETB  F_DSRD             ;read result in next 16th sleep window
READDS2:RET                      ;(no need wait 750mS, 16*500mS=8S)

DSRES: 	ORL   P0DIR,#010H        ;reset pulse for ds18b20
	MOV   R2,#04H            ;release w1 & wait before
DSRES1:	NOP
	DJNZ  R2,DSRES1
	MOV   C,P0.4             ;check if w1 inactive(high)
	CPL   C
	JC    DSRES5             ;if active exit with error
        CLR   P0.4
	ANL   P0DIR,#0EFH        ;set w1 low
	MOV   R2,#080H           ;wait ~500uS
DSRES2: NOP
	DJNZ  R2,DSRES2
	ORL   P0DIR,#010H        ;release w1
	SETB  P0.4
	MOV   R2,#015H           ;wait ~80uS
DSRES3: NOP
	DJNZ  R2,DSRES3
	MOV   C,P0.4             ;check presence
	MOV   R2,#06BH           ;wait to complete read slot 
DSRES4: NOP
	DJNZ  R2,DSRES4
	JNC   DSRES6             ;if no presence 
DSRES5:	MOV   TEMPL,#0FFH        ;set invalid temp -0.0
	MOV   TEMPH,#0FFH
DSRES6: 
	RET	

DSCOMM: ORL   P0DIR,#010H        ;command to ds18b20
	CLR   P0.4
	MOV   R3,#08H
DSCOMM1:RRC   A                  ;for all bits
	ANL   P0DIR,#0EFH        ;set w1 low
	JNC   DSCOMM2 
	ORL   P0DIR,#010H        ;if 1 release w1 after ~10uS
DSCOMM2:MOV   R2,#0EH
DSCOMM3:NOP
	DJNZ  R2,DSCOMM3         ;wait to complete write slot ~60uS
	ORL   P0DIR,#010H        ;release w1
	CJNE  A,#00,$+3          ;wait
	DJNZ  R3,DSCOMM1	 ;next bit
	RET

DSREAD: ORL   P0DIR,#010H        ;read ds18b20
	CLR   P0.4
	CLR   A
	MOV   R3,#08H
DSREAD1:ANL   P0DIR,#0EFH        ;for all bits
        NOP                      ;set w1 low
	ORL   P0DIR,#010H        ;release w1 after ~10uS
	SETB  P0.4
	CJNE  A,#00,$+3          ;wait ~20-30uS
	CJNE  A,#00,$+3
	MOV   C,P0.4             ;read data bit from ds
	RRC   A
	CLR   P0.4
        MOV   R2,#0EH            ;wait to complete read slot ~60uS
DSREAD3:NOP
	DJNZ  R2,DSREAD3
	DJNZ  R3,DSREAD1	 ;next bit
	RET

;******************************************************;
;CLCRC - Calculate CRC R2 bytes @DPTR used B
;On exit: R0 - CRC low, R1 - CRC high
;******************************************************;
CLCRC:  MOV   R0,#0FFH           ;L(initialize CRC by 0ffh)
        MOV   R1,#0FFH           ;H
CLCRC1: MOVX  A,@DPTR
	INC   DPTR
        XCH   A,R0               ;calculate CRC
        XCH   A,R1               ;R0 - low
        MOV   B,A                ;R1 - high
        LCALL CRC_TL
        XRL   A,R0
        MOV   R0,A
        MOV   A,B
        LCALL CRC_TH
        XRL   A,R1
        MOV   R1,A
	DJNZ  R2,CLCRC1
	RET	

;******************************************************;
;CPCRC - Calculate CRC from DPTR up to R2:R3, used B
;On exit: R0 - CRC low, R1 - CRC high
;******************************************************;
CPCRC:  MOV   R0,#0FFH           ;L(initialize CRC by 0ffh)
        MOV   R1,#0FFH           ;H
CPCRC1: CLR   A
	MOVC  A,@A+DPTR
	INC   DPTR
        XCH   A,R0               ;calculate CRC
        XCH   A,R1               ;R0 - low
        MOV   B,A                ;R1 - high
        LCALL CRC_TL
        XRL   A,R0
        MOV   R0,A
        MOV   A,B
        LCALL CRC_TH
        XRL   A,R1
        MOV   R1,A
	MOV   A,DPH
	XRL   A,R2
	JNZ   CPCRC1
	MOV   A,DPL
	XRL   A,R3
	JNZ   CPCRC1
	RET

CRC_TL:
        JZ    CRC_TL1
        MOVC  A,@A+PC
CRC_TL1:
        RET
        .DB 21H,42H,63H,84H,0A5H,0C6H,0E7H,8,29H,4AH,6BH,8CH
        .DB 0ADH,0CEH,0EFH,31H,10H,73H,52H,0B5H,94H,0F7H,0D6H
        .DB 39H,18H,7BH,5AH,0BDH,9CH,0FFH,0DEH,62H,43H,20H,1
        .DB 0E6H,0C7H,0A4H,85H,6AH,4BH,28H,9,0EEH,0CFH,0ACH,8DH
        .DB 53H,72H,11H,30H,0D7H,0F6H,95H,0B4H,5BH,7AH,19H,38H
        .DB 0DFH,0FEH,9DH,0BCH,0C4H,0E5H,86H,0A7H,40H,61H,2,23H
        .DB 0CCH,0EDH,8EH,0AFH,48H,69H,10,2BH,0F5H,0D4H,0B7H
        .DB 96H,71H,50H,33H,12H,0FDH,0DCH,0BFH,9EH,79H,58H,3BH
        .DB 1AH,0A6H,87H,0E4H,0C5H,22H,3,60H,41H,0AEH,8FH,0ECH
        .DB 0CDH,2AH,11,68H,49H,97H,0B6H,0D5H,0F4H,13H,32H,51H
        .DB 70H,9FH,0BEH,0DDH,0FCH,1BH,3AH,59H,78H,88H,0A9H,0CAH
        .DB 0EBH,12,2DH,4EH,6FH,80H,0A1H,0C2H,0E3H,4,25H,46H
        .DB 67H,0B9H,98H,0FBH,0DAH,3DH,1CH,7FH,5EH,0B1H,90H,0F3H
        .DB 0D2H,35H,14H,77H,56H,0EAH,0CBH,0A8H,89H,6EH,4FH,2CH
        .DB 13,0E2H,0C3H,0A0H,81H,66H,47H,24H,5,0DBH,0FAH,99H
        .DB 0B8H,5FH,7EH,1DH,3CH,0D3H,0F2H,91H,0B0H,57H,76H,15H
        .DB 34H,4CH,6DH,14,2FH,0C8H,0E9H,8AH,0ABH,44H,65H,6,27H
        .DB 0C0H,0E1H,82H,0A3H,7DH,5CH,3FH,1EH,0F9H,0D8H,0BBH
        .DB 9AH,75H,54H,37H,16H,0F1H,0D0H,0B3H,92H,2EH,15,6CH
        .DB 4DH,0AAH,8BH,0E8H,0C9H,26H,7,64H,45H,0A2H,83H,0E0H
        .DB 0C1H,1FH,3EH,5DH,7CH,9BH,0BAH,0D9H,0F8H,17H,36H,55H
        .DB 74H,93H,0B2H,0D1H,0F0H

CRC_TH:
        JZ    CRC_TH1
        MOVC  A,@A+PC
CRC_TH1:
        RET
        .DB 10H,20H,30H,40H,50H,60H
        .DB 70H,81H,91H,0A1H,0B1H,0C1H,0D1H,0E1H,0F1H,12H,2,32H
        .DB 22H,52H,42H,72H,62H,93H,83H,0B3H,0A3H,0D3H,0C3H,0F3H
        .DB 0E3H,24H,34H,4,14H,64H,74H,44H,54H,0A5H,0B5H,85H
        .DB 95H,0E5H,0F5H,0C5H,0D5H,36H,26H,16H,6,76H,66H,56H
        .DB 46H,0B7H,0A7H,97H,87H,0F7H,0E7H,0D7H,0C7H,48H,58H
        .DB 68H,78H,8,18H,28H,38H,0C9H,0D9H,0E9H,0F9H,89H,99H
        .DB 0A9H,0B9H,5AH,4AH,7AH,6AH,1AH,10,3AH,2AH,0DBH,0CBH
        .DB 0FBH,0EBH,9BH,8BH,0BBH,0ABH,6CH,7CH,4CH,5CH,2CH,3CH
        .DB 12,1CH,0EDH,0FDH,0CDH,0DDH,0ADH,0BDH,8DH,9DH,7EH
        .DB 6EH,5EH,4EH,3EH,2EH,1EH,14,0FFH,0EFH,0DFH,0CFH,0BFH
        .DB 0AFH,9FH,8FH,91H,81H,0B1H,0A1H,0D1H,0C1H,0F1H,0E1H
        .DB 10H,0,30H,20H,50H,40H,70H,60H,83H,93H,0A3H,0B3H,0C3H
        .DB 0D3H,0E3H,0F3H,2,12H,22H,32H,42H,52H,62H,72H,0B5H
        .DB 0A5H,95H,85H,0F5H,0E5H,0D5H,0C5H,34H,24H,14H,4,74H
        .DB 64H,54H,44H,0A7H,0B7H,87H,97H,0E7H,0F7H,0C7H,0D7H
        .DB 26H,36H,6,16H,66H,76H,46H,56H,0D9H,0C9H,0F9H,0E9H
        .DB 99H,89H,0B9H,0A9H,58H,48H,78H,68H,18H,8,38H,28H,0CBH
        .DB 0DBH,0EBH,0FBH,8BH,9BH,0ABH,0BBH,4AH,5AH,6AH,7AH
        .DB 10,1AH,2AH,3AH,0FDH,0EDH,0DDH,0CDH,0BDH,0ADH,9DH
        .DB 8DH,7CH,6CH,5CH,4CH,3CH,2CH,1CH,12,0EFH,0FFH,0CFH
        .DB 0DFH,0AFH,0BFH,8FH,9FH,6EH,7EH,4EH,5EH,2EH,3EH,14
        .DB 1EH

INITRFDAT:
	.DB 2,20H,0CH            ;Enable crc, crc 2 byte, PTX, power down
	.DB 2,24H,0FFH           ;uS 4000, 16 retransmit
	.DB 2,26H,027H           ;27 RF_SETUP 250kbit 0dBm
	.DB 2,22H,03H            ;Enable data pipe 0,1
	.DB 2,23H,03H            ;ADDR 5 bytes
	.DB 2,21H,00H            ;Disable auto acknowledgement data pipe 0-5
;
	.DB 6,2AH,ARFID,0F0H,0F0H,0F0H,0F0H ;rx addr
	.DB 6,30H,ARFID,0F0H,0F0H,0F0H,0F0H ;tx addr
	.DB 6,2BH,0D2H,0F0H,0F0H,0F0H,0F0H ;RX_ADDR_P1
;
	.DB 2,31H,RFRXRWP        ;20H            ;RX_PW_P1 32 bytes in pipe 0
	.DB 2,32H,RFRXRWP        ;20H            ;RX_PW_P1 32 bytes in pipe 1
	.DB 2,25H,RFCH           ;channel 100
	.DB 2,3DH,00H            ;feature
	.DB 2,3CH,00H            ;DYNPD dynamic payload for all pipes disabled
	.DB 2,20H,0EH            ;Enable crc, crc 2 byte, PTX, power up
	.DB 0

        .org 17FBH
	LJMP 0H

        .org 3F80H
        .DB 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .DB 0,0,0,0,0,0,0,0,0,0,0,0,0,2,0,0
PFLASH: CLR   IEN0.7
	MOV   CLKCTRL,#00H
	MOV   R1,#04H
PFLASH1:NOP
	DJNZ  R1,PFLASH1
	SETB  FSR.5
	MOV   A,#NRFWOFS
	CLR   C
	RRC   A
	MOV   R0,A
	CLR   A
PFLASH2:MOV   FCR,A              ;erase page
        INC   A
	CJNE  A,00H,PFLASH2
	ORL   PCON,#10H
	MOV   DPH,#NRFWOFS       ;fw loaded memory
	MOV   DPL,#00H
	MOV   DPH1,#00H          ;begin of code
	MOV   DPL1,#00H
PFLASH3:MOVX  A,@DPTR            ;copy from loaded 
	MOV   DPS,#01H           ;to code memory
	MOVX  @DPTR,A
	INC   DPTR
	MOV   DPS,#00H
	INC   DPTR
	MOV   A,DPH
	CJNE  A,#NAMTABH,PFLASH3
	ANL   PCON,#0EFH
	CLR   FSR.5
	LJMP  0H
        .org 3FE0H
        .DB 0,0,0,0,0,0,0,0,0,0,0,0,0,2,0,0
        .org 3FFFH
	.DB 0FFH
        .end

