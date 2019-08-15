;******************************************************;
;   NRF24LE1 <-> ESP8266, RS <-> RF control program    ;
;   Lutov Andrey, Donetsk                              ;
;******************************************************;
;               For quartz 16 MHz                      ;
;******************************************************;
;                addresses map:                        ;
;         ----------------------------                 ;
; 00000-017FFh - running firmware   (6144bytes)        ;
; 01800-027FFh - store new firmware via RS (6144bytes) ;
; 03000-037FFh - switch names table                    ;
; 03800-03F7Fh - program flash not used                ;
; 03F80-03FFFh - flash program proc                    ;
; 00000-001FFh - retentive XRAM  (RF RX TX buffers)    ;
; 00200-003FFh - non retentive XRAM                    ;
; 0FA00-0FBFFh - extended endurance NVRAM (DEL comm)   ;
; 0FC00-0FFFFh - NVRAM (not used)                      ;
;******************************************************;
;              SW addresses in ext RAM:                ;
; 0000-00FFh - 256 bytes FR data 4(dword)*4(parnum)*16 ;
; 0100-01FF  - various buffers                         ;
; 0200-02FFh - RS receive buffer                       ;
; 0300-03FFh - not used                                ;
;******************************************************;
; NRF continuously sends maximum of 127 frames         ;
; (127*(~1mS TX + ~8mS RX)=~1.1S) for each of 16       ;
; remote nodes until receive response from it          ;
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
;******************************************************;

.nolist
.include equnrf24.inc
.list
; nrfid
RFWID    .equ 00H                ;device ID: 00H for gateway, 01H for remote switch
RRFID    .equ 00H                ;device ADDR:  00H for nrf1, 01H for nrf2... 08H for nrf9
NRFID    .equ RRFID+31H
;RF values
RFNRETR  .equ 0FEH               ;max_rf_retr_count*2
RFRXRWP  .equ 018H               ;number of payload bytes in RF frame
;RFCH     .equ (RRFID*2)+066H     ;RF channel
;RFCH     .equ ((RRFID-4)*2)+066H     ;RF channel
RFCH     .equ 064H               ;RF channel
;
;Timers values
;for 16MHz
HI_TIMER .equ 05DH               ;32Hz timer Int frequency high divisor
LO_TIMER .equ 03CH               ;low(16M/12/32=41667=A2C3. FFFF-A2C3=5D3C)
;RF
HI_T250U .equ 0FEH               ;250uS
LO_T250U .equ 0B2H
HI_T2M   .equ 0F5H               ;2MS
LO_T2M   .equ 094H
HI_T4M   .equ 0EBH               ;4MS
LO_T4M   .equ 02AH
HI_T8M   .equ 0D6H               ;8MS
LO_T8M   .equ 054H
HI_T16M  .equ 0A9H               ;16MS
LO_T16M  .equ 031H
HI_T32M  .equ 052H               ;32MS
LO_T32M  .equ 063H
HI_T30U  .equ 0FFH               ;30uS
LO_T30U  .equ 0D7H
;
RFRXBUF  .equ 100H               ;32 bytes rf rx buf
RFRXBUFH .equ 01H                ;32 bytes rf rx buf
RFRXBUFL .equ 00H                ;32 bytes rf rx buf
RFTXBUF  .equ 120H               ;32 bytes rf tx buf
RFTXBUFH .equ 01H                ;32 bytes rf tx buf
RFTXBUFL .equ 20H                ;32 bytes rf tx buf
RSINBUF  .equ 140H               ;128 bytes rs input line
RSOUTBUF .equ 1C0H               ;64 bytes rs output line
;ROM NAMTAB values
NRFWOFS  .equ 18H                ;address to load fw
NAMTABH  .equ 30H                ;names tab in program memory
NAMSZ    .equ 20H                ;name size in NAMTAB
; internal RAM
;Bits
;
;Bytes
RSRHEAD  .equ 16H                ;R6 in RSINT RS buffer head pointer
RSRTAIL  .equ 17H                ;R7 in RSINT RS buffer tail pointer
RSID     .equ 13H                ;R3 in RSINT current id to send via rs
RSTMR    .equ 12H                ;R2 in RSINT timer for polling
RS_RXER  .equ 11H                ;R1 in RSINT RX error counter
;
RSTO     .equ 20H                ;RS timeout
PRFID    .equ 21H                ;next polling id
RFID     .equ 22H                ;current rfid
FWID     .equ 23H
FWADRL   .equ 24H
FWADRH   .equ 25H
;
RFBITM   .equ 26H                ;(RF bits(1) + RFLTO(1)) values for 16 devices 2*16=32 bytes
TEMPBIN  .equ 46H                ;DECBIN 46H-49H, .HEX 46H-5AH
TBINDEC  .equ 5cH                ;BINDEC 5c-5f
; internal RAM      
MAIN:
	.org 0h
	NOP
        AJMP  MAIN1
	.org 3h
	RETI
	.org 06h
	.DB RFWID                ;FW id
	.DB RRFID                ;RF id
;	.DB 00H,00H              ;
	.org 0bh
        AJMP  TI0INT
	.org 13h
        RETI
	.org 1bh
        AJMP  TI1INT
	.org 23h
        AJMP  RSINT
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
	.text RS232<->NRF24_
VERNRF	.TEXT v0.00_15.08.19
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
        MOV   TH1,#HI_TIMER      ;timer1- int 32 Hz
        MOV   TL1,#LO_TIMER      ;
        SETB  TCON.6             ;restart timer1
        MOV   R0,#7FH            ;Yes, get 1 byte on stack top
        MOV   A,@R0              ;
        XRL   A,#11H             ;stack overflow?
        JZ    TI1INT2            ;no
TI1INT1:
        POP   ACC                ;yes, halt
        POP   PSW                ;restart
        LJMP  MAIN
TI1INT2:
        DEC   R0
        MOV   A,@R0              ;get 2 byte on stack top
        JNZ   TI1INT1
	INC   R7
	MOV   A,R7               ;32Hz
	ANL   A,#03H             ;32/4=0.125
	JNZ   TI_INT3            ;
        MOV   A,RSTMR            ;update wait counter
        JZ    TI_INT3            ;
        DEC   RSTMR              ;
TI_INT3:
TI1INTE:
        POP   ACC                ;restore registers
        POP   PSW                ;and return
        RETI                     ;

;******************************************************;
RSINT:                 ;RS232 interrupt routine
;******************************************************;
        PUSH  PSW                ;save used registers
        PUSH  ACC
	PUSH  MPAGE
        MOV   PSW,#10H           ;select bank 2 (10H-17H) 
        JNB   S0CON.0,RSINT6     ;RX int?
	MOV   A,RSRHEAD          ;get head pointer
	INC   A
	CJNE  A,RSRTAIL,RSINT1   ;if buffer full - ignore next byte
	MOV   A,RS_RXER          ;inc error counter
	INC   A
	JZ    RSINT3
	MOV   RS_RXER,A
	SJMP  RSINT3
RSINT1:
	XCH   A,RSRHEAD
	MOV   R0,A
	MOV   MPAGE,#02H
	MOV   A,S0BUF
	MOVX  @R0,A
RSINT3:
	CLR   S0CON.0            ;clear receive int flag
RSINT6:
	JNB   S0CON.1,RSINTE     ;tx int?
	MOV   A,R4               ;yes
	JZ    RSINT10
	MOV   MPAGE,A            ;get pointer to next byte
	MOV   A,R5
	INC   A
	MOV   R0,A
	JNZ   RSINT7
	INC   MPAGE
RSINT7: MOVX  A,@R0
	JNZ   RSINT9             ;zero end of line
	MOV   R5,A               ;if nothing to send
	MOV   R4,A
	SJMP  RSINT10
RSINT9:
	MOV   S0BUF,A            ;send byte
	MOV   A,R0
	MOV   R5,A               ;60  save pointer to next byte
	MOV   A,MPAGE
	MOV   R4,A
RSINT10:
	CLR   S0CON.1            ;clear transmit int flag
RSINTE:
	POP   MPAGE
        POP   ACC
        POP   PSW
        RETI

;******************************************************;
TI0INT:                 ;Timer interrupt 0 routine
;******************************************************;
        PUSH  PSW                ;save contents
        PUSH  ACC
	PUSH  MPAGE
        MOV   PSW,#18H           ;select bank 3 (18H-1FH) 
	CLR   TCON.4
	MOV   A,R7
	JZ    TI0INT5            ;if RF inactive
	INC   A
	JZ    TI0INT5            ;if RF send complete wait wor next rfid
	DEC   A
        CJNE  A,#RFNRETR,$+3
	JC    TI0INT1            ;if max retransmission
	CLR   P1.3               ;RX disable
	CLR   P1.4               ;TX disable
	CLR   RFCON.0            ;0-set TX mode
        CLR   RFCON.1
	MOV   A,#20H
	LCALL BSNSPI
	MOV   A,#0EH
	LCALL BSNSPI
	SETB  RFCON.1
	MOV   R7,#0FFH           ;next rfid
	SJMP  TI0INT5
TI0INT1:JNB   ACC.0,TI0INT2
;
	CLR   P0.6
;
	CLR   RFCON.0            ;0-set TX mode
	CLR   P1.3               ;RX disable
	CLR   P1.4               ;TX disable
        CLR   RFCON.1
	MOV   A,#20H
	LCALL BSNSPI
	MOV   A,#0EH
	LCALL BSNSPI
	SETB  RFCON.1
        MOV   TH0,#HI_T30U
        MOV   TL0,#LO_T30U
	INC   R7
        SETB  TCON.4             ;start timer0
	SETB  IEN0.1             ;en timer0
	SJMP  TI0INTE
TI0INT2:MOV   MPAGE,#RFTXBUFH
	MOV   R0,#RFTXBUFL	
	MOV   R1,#RFRXRWP
	SETB  P1.4               ;TX enable
	CLR   RFCON.1
	MOV   A,#0A0H
	LCALL BSNSPI
TI0INT3:MOVX  A,@R0
	LCALL BSNSPI
	MOV   A,R0
	INC   A
	MOV   R0,A
	JNZ   TI0INT4
	INC   MPAGE
TI0INT4:DJNZ  R1,TI0INT3	
	SETB  RFCON.1
	SETB  RFCON.0
	INC   R7
TI0INT5:CLR   TCON.4             ;stop timer0
	CLR   IEN0.1             ;dis timer0
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
	JNB   ACC.6,RFINT5       ;yes
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
	MOV   MPAGE,#RFTXBUFH
	MOV   R0,#RFTXBUFL
	MOVX  A,@R0              ;TX_ADDR
	ANL   A,#7FH
	XRL   A,R2	
	JNZ   RFINT4
;
	CLR   P0.6
	SETB  P0.0
;
	CLR   TCON.4             ;stop TF0
	CLR   IEN0.1
	CLR   RFCON.0            ;idle RF
	MOV   R7,#0FFH           ;idle TF0 next rfid
	MOV   R6,#0FFH           ;but RX busy
RFINT4:
	MOV   A,#40H             ;clear RX_DR
	SJMP  RFINT6
RFINT5: MOV   C,ACC.4            ;TX int? MAX_RT
	ORL   C,ACC.5            ;TX_DS
	JNC   RFINTE
;
	SETB  P0.6
	CLR   P0.0
;
	CLR   P1.4               ;TX disable
	MOV   MPAGE,#RFRXBUFH
	MOV   R0,#RFRXBUFL
	MOV   A,#0FFH
	MOVX  @R0,A              ;set invalid RX frid
        CLR   RFCON.1            ;RX mode
	MOV   A,#20H	
	LCALL BSNSPI
	MOV   A,#0FH
	LCALL BSNSPI
	SETB  RFCON.1
	SETB  P1.3               ;RX enable
	CLR   TCON.4
        MOV   TH0,#HI_T8M
        MOV   TL0,#LO_T8M
        SETB  TCON.4             ;restart timer0
	SETB  IEN0.1             ;en timer0
	MOV   A,#30H             ;clear TX_DS+MAX_RT
RFINT6: MOV   R2,A
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

STRFADR:MOV   A,RFID
	ANL   A,#0FH
	ORL   A,#0E0H
	MOV   R0,A
        CLR   RFCON.1
	MOV   A,#2AH	
	ACALL BSNSPI
	MOV   A,R0
	ACALL BSNSPI
	MOV   R1,#04H
STRFADR1:
	MOV   A,#0F0H
	ACALL BSNSPI
	DJNZ  R1,STRFADR1	
	SETB  RFCON.1
	MOV   R1,#04H
STRFADR2:
	NOP
	DJNZ  R1,STRFADR2
        CLR   RFCON.1
	MOV   A,#30H	
	ACALL BSNSPI
	MOV   A,R0
	ACALL BSNSPI
	MOV   R1,#04H
STRFADR3:
	MOV   A,#0F0H
	ACALL BSNSPI
	DJNZ  R1,STRFADR3	
	SETB  RFCON.1
	RET

;******************************************************;
;little wait for rfinit proc only
;******************************************************;
TWAIT: 	MOV   R1,#0FFH
TWAIT1: NOP
	NOP
 	DJNZ  R1,TWAIT1
	RET			


;******************************************************;
MAIN1:                          ;Start after RESET
;******************************************************;
	MOV   CLKCTRL,#00H
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
	MOV   DPTR,#0            ;clear xram
MAIN3:
        CLR   A
	MOVX  @DPTR,A
	INC   DPTR
	MOV   A,DPH
	CJNE  A,#04H,MAIN3
        MOV   SP,#60H            ;set stack
;
        MOV   A,#00H
        MOV   RFID,A
        MOV   PRFID,A            ;set rfid, prfid

	MOV   R0,#RFBITM         ;initially
	INC   R0                 ;set norfl for all clients
	MOV   R1,#10H
	MOV   A,#0FH
MAIN4:  MOV   @R0,A
	INC   R0
	INC   R0
	DJNZ  R1,MAIN4	

;uart
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
;ports
	MOV   P0DIR,#094H        ;p0:0,1,3,5,6 out, 2,4,7 in
	MOV   P1DIR,#0E7H        ;p1 3,4 out 0-2,5-7 in
;adc
	MOV   ADCCON1,#38H       ;1.2V ref +1/3VDD
	MOV   ADCCON2,#00H       ;
	MOV   ADCCON2,#0C0H      ;12 bit
;power 
        MOV   POFCON,#0E0H       ;2.7v
;rtc2
	MOV   CLKLFCTRL,#1
	MOV   RTC2CMP0,#0FFH
	MOV   RTC2CMP1,#0FFH
	MOV   RTC2CON,#06H       ;not reset #06H
	ANL   RTC2CON,#0FEH
;ints
	SETB  IEN0.7             ;en all
	SETB  IEN0.3             ;en timer1
	SETB  IEN0.4             ;en serial
	SETB  IEN1.5             ;rtc2 TICK
	SETB  IEN1.1             ;RFIRQ
;
	CLR   P0.0
	CLR   P0.1
	CLR   P0.6
	CLR   P1.3               ;RX dis
	CLR   P1.4               ;TX dis
;rf
MAIN5:
        LCALL RFINIT             ;RF initialisation
	LCALL STRFADR
;main loop code
MAIN6:
	LCALL RFSEND
	LCALL RFREAD
	LCALL RFNXID
	LCALL SILIN              ;out:A-readed count
	LCALL SOLIN              ;in:A-readed count
	SJMP  MAIN6

;******************************************************;
; SILIN read ASCII line from RS buf to RSINBUF
; On exit A - readed symbols
;******************************************************;
SILIN:  PUSH  MPAGE
	CLR   A                  ;if ack for previous command
	CJNE  A,14H,SILINE       ;not yet sended - exit
	MOV   MPAGE,#02H
	MOV   R0,RSRTAIL
SILIN1:	CLR   A		
	MOV   R2,A
	MOV   DPTR,#RSINBUF
	MOVX  @DPTR,A
SILIN2:	MOV   RSRTAIL,R0         ;fix buffer at beginning of prefix
	MOV   DPTR,#NRPRFX       ;searching for prefix
SILIN3:	CLR   A
	MOVC  A,@A+DPTR
	JZ    SILIN5             ;end of prefix?
	MOV   R1,A               
	INC   DPTR
	MOV   A,R0
	XRL   A,RSRHEAD          ;buffer empty?
	JZ    SILINE             ;yes, exit
	MOVX  A,@R0
	INC   R0
	XRL   A,R1               ;compare symbol
	JZ    SILIN3             ;ok, next
	SJMP  SILIN2             ;no,remove symbols from buffer
SILIN5:	MOV   DPTR,#RSINBUF      ;prefix ok store line without prefix
SILIN6:	MOV   A,R0
	XRL   A,RSRHEAD          ;buffer empty?(not all line received yet)
	JZ    SILINE             ;if empty exit leaving begin of line in buffer
	MOVX  A,@R0
	INC   R0
	CJNE  A,#20H,$+3         ;if <20H - <CR> or <LF>
	JC    SILIN8
	MOVX  @DPTR,A            ;store in buffer
	INC   DPTR               ;next pointers
	INC   R2
	MOV   A,R2
	CJNE  A,#7EH,$+3         ;max in buf size reached?
	JC    SILIN6             ;no, next
	SJMP  SILIN1	         ;yes, invalid data ignored
SILIN8: CLR   A
	MOVX  @DPTR,A            ;store zero end
	MOV   RSRTAIL,R0         ;save new tail pointer
	MOV   A,R2
SILINE: POP   MPAGE
	RET

;******************************************************;
; SOLIN write ASCII line to RS output buf
; On entry A - readed symbols by SILIN
;******************************************************;
SOLIN:  PUSH  MPAGE
	MOV   R7,A               ;buffer symbol count
	MOV   A,14H
	JNZ   SOLIN2
	MOV   A,R7
	JZ    SOLIN0
	MOV   DPS,#01H
	MOV   DPTR,#NRECTX
	MOV   DPS,#00H
	MOV   DPTR,#RSINBUF
	LCALL STPCOM             ;nrfcomm?
	JZ    SOLIN0             ;goto if .
	MOV   DPS,#01H
	MOV   DPTR,#NRCMTX
	MOV   DPS,#00H
	MOV   DPTR,#RSINBUF
	LCALL STPCOM             ;nrfcomm?
	JZ    SOLIN1             ;goto if command
	LJMP  SOLIN24            ;goto nrfresp
SOLIN0: LJMP  SOLIN53
SOLIN1:	LCALL SKPSPC             ;yes, skip spaces
	JC    SOLIN4
	MOV   R3,DPH             ;save pointer
	MOV   R4,DPL	
	MOV   DPS,#01H
	MOV   DPTR,#NRCMGT
	MOV   DPS,#00H
	LCALL STPCOM             ;get?
	JNZ   SOLIN5
	LCALL GTPAR              ;yes, get unit and par number
	MOV   R4,A
	INC   A
	JZ    SOLIN4             ;if error
	LCALL STPRRS
	MOV   A,#20H             ;space
	MOVX  @DPTR,A
	INC   DPTR
	MOV   A,R4
	SWAP  A                  ;ddii00dd
	RL    A                  ;dii00ddd
	MOV   R0,A
	ANL   A,#07H
	ADD   A,#NAMTABH
	MOV   DPH1,A
	MOV   A,R0
	ANL   A,#0E0H            ;begin of record
	MOV   DPL1,A
	MOV   DPS,#01H
	CLR   A
	MOVC  A,@A+DPTR
	MOV   DPS,#00H
	JZ    SOLIN3             ;if byte 00 or 0ff
	INC   A
	JZ    SOLIN3             ;noname
	LCALL MOVPDAT            ;copy name
	LJMP  SOLIN72            ;goto close&send line
SOLIN2:	LJMP  SOLINE             ;no resp, exit
SOLIN3: MOV   DPS,#01H
	MOV   DPTR,#NRNNTX       ;put noname
	MOV   DPS,#00H
	LCALL MOVPDAT
	LJMP  SOLIN72            ;goto close&send line
SOLIN4: LJMP  SOLIN36            ;goto error
SOLIN5: MOV   DPH,R3
	MOV   DPL,R4	
	MOV   DPS,#01H
	MOV   DPTR,#NRCMAD
	MOV   DPS,#00H
	LCALL STPCOM
	JNZ   SOLIN8
	LCALL GTPAR              ;add
	MOV   R4,A
	INC   A
	JZ    SOLIN4             ;err
	MOVX  A,@DPTR
	CJNE  A,#20H,SOLIN4      ;if no space - error
	INC   DPTR
	MOVX  A,@DPTR
	CJNE  A,#21H,$+3
	JC    SOLIN4             ;if no name to store - error
	MOV   A,R4               ;00ddddii
	SWAP  A                  ;ddii00dd
	RL    A                  ;dii00ddd
	MOV   R0,A
	ANL   A,#07H
	ADD   A,#NAMTABH
	MOV   DPH1,A
	MOV   A,R0
	ANL   A,#0E0H            ;begin of record
	MOV   DPL1,A
	MOV   DPS,#01H
	CLR   A
	MOVC  A,@A+DPTR
	MOV   DPS,#00H
	INC   A
	JNZ   SOLIN4             ;if byte <> 0ff error
	CLR   IEN0.7
	SETB  FSR.5
	MOV   R1,#1FH
SOLIN6:	MOVX  A,@DPTR
	INC   DPTR
	CJNE  A,#20H,SOLIN6A     ;space in name not allowed
	MOV   A,#5FH             ;replace space by _
SOLIN6A:JC    SOLIN7
	MOV   DPS,#01H
	ORL   PCON,#10H
	MOVX  @DPTR,A
	ANL   PCON,#0EFH
	INC   DPTR
	MOV   DPS,#00H
	DJNZ  R1,SOLIN6		
SOLIN7: CLR   A
	MOV   DPS,#01H
	ORL   PCON,#10H
	MOVX  @DPTR,A
	ANL   PCON,#0EFH
	MOV   DPS,#00H
	CLR   FSR.5
	SETB  IEN0.7
	LJMP  SOLIN35

SOLIN8: MOV   DPH,R3
	MOV   DPL,R4	
	MOV   DPS,#01H
	MOV   DPTR,#NRCMDL
	MOV   DPS,#00H
	LCALL STPCOM
	JZ    SOLIN10
	LJMP  SOLIN16
SOLIN9:	LJMP  SOLIN36
SOLIN10:LCALL GTPAR
	MOV   R4,A
	INC   A
	JZ    SOLIN9
	MOV   A,R4
	SWAP  A                  ;ddii00dd
	MOV   R1,A
	RL    A                  ;dii00ddd
	MOV   R0,A
	ANL   A,#07H
	ADD   A,#NAMTABH
	MOV   R2,A
	MOV   A,R0
	ANL   A,#0E0H            ;begin of record
	MOV   R3,A               ;R2:R3- begin of record
	MOV   A,R0
	ANL   A,#06H
	ADD   A,#NAMTABH         ;begin of page
	MOV   DPH,A
	MOV   R4,A               ;R4- begin of page
	ADD   A,#02H             ;next page
	MOV   R5,A               ;R5 - next page
	MOV   DPL,#00H
	MOV   DPH1,#0FAH         ;temp memory @0FA00H
	MOV   DPL1,#00H
	CLR   IEN0.7
	SETB  FSR.5
	MOV   FCR,#32            ;erase temp
	MOV   FCR,#33            ;memory
	MOV   DPS,#00H
	MOV   A,R2               ;verify if begin of record
	XRL   A,R4               ;equal to begin of page
	JNZ   SOLIN11
	MOV   A,R3               ;R3=0?
	JZ    SOLIN12            ;if yes, skip 1st step
SOLIN11:CLR   A                  ;1st step
	MOVC  A,@A+DPTR          ;copy from begin of page
	MOV   DPS,#01H           ;to begin of record
	MOVX  @DPTR,A
	INC   DPTR
	MOV   DPS,#00H
	INC   DPTR
	MOV   A,DPH
	XRL   A,R2
	JNZ   SOLIN11
	MOV   A,DPL
	XRL   A,R3
	JNZ   SOLIN11
SOLIN12:MOV   R0,#20H            ;2nd step 
SOLIN13:INC   DPTR               ;skip record (32 bytes)
	MOV   DPS,#01H
	INC   DPTR
	MOV   DPS,#00H
	DJNZ  R0,SOLIN13
SOLIN14:CLR   A                  ;3rd step
	MOVC  A,@A+DPTR          ;copy from next record 
	MOV   DPS,#01H           ;to the end of page
	MOVX  @DPTR,A
	INC   DPTR
	MOV   DPS,#00H
	INC   DPTR
	MOV   A,DPH
	XRL   A,R5
	JNZ   SOLIN14
	MOV   A,R1
	ANL   A,#03H
	MOV   R1,A
	MOV   A,#NAMTABH
	CLR   C
	RRC   A
	ADD   A,R1
	MOV   FCR,A              ;erase page
	MOV   DPH,#0FAH         ;temp memory @0FA00H
	MOV   DPL,#00H
	MOV   DPH1,R4           ;begin of page
	MOV   DPL1,#00H
SOLIN15:MOVX  A,@DPTR            ;copy from next record 
	MOV   DPS,#01H           ;to the end of page
	ORL   PCON,#10H
	MOVX  @DPTR,A
	ANL   PCON,#0EFH
	INC   DPTR
	MOV   DPS,#00H
	INC   DPTR
	MOV   A,DPH
	CJNE  A,#0FCH,SOLIN15
	CLR   FSR.5
	SETB  IEN0.7
	LJMP  SOLIN35

SOLIN16:MOV   DPH,R3
	MOV   DPL,R4	
	MOV   DPS,#01H
	MOV   DPTR,#NRFWLD
	MOV   DPS,#00H
	LCALL STPCOM
	JZ    SOLIN17
	LJMP  SOLIN53
SOLIN16A:
	LJMP  SOLIN36
SOLIN17:
	LCALL GTNUM
	MOV   R4,A
	INC   A
	JZ    SOLIN16A
	MOV   DPL,#00H
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
	JNZ   SOLIN16A
	CLR   A
	MOVC  A,@A+DPTR
	XRL   A,R1
	JNZ   SOLIN16A
	MOV   A,R4
	CJNE  A,#11H,$+3
	JNC   SOLIN16A
	JNZ   SOLIN18
	MOV   DPTR,#06H
	CLR   A
	MOVC  A,@A+DPTR            ;check 6,7
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
	JNZ   SOLIN16A
	CLR   A
	MOVC  A,@A+DPTR
	XRL   A,R1
	JNZ   SOLIN16A
        LCALL STPRRS
	MOV   A,#20H             ;space
	MOVX  @DPTR,A
	INC   DPTR
	MOV   DPS,#01H
	MOV   DPTR,#NRUPTX       ;Updating
	MOV   DPS,#00H
	LCALL MOVPDAT
	MOV   A,#0DH
	MOVX  @DPTR,A
	INC   DPTR
	MOV   A,#0AH
	MOVX  @DPTR,A
	INC   DPTR
	CLR   A
	MOVX  @DPTR,A
	MOV   DPTR,#RSOUTBUF
	MOVX  A,@DPTR
	MOV   15H,DPL
	MOV   14H,DPH
	MOV   S0BUF,A
SOLIN17A:
	MOV   A,14H
	JNZ   SOLIN17A
	LJMP  PFLASH
SOLIN18:
	CLR   A
	MOV   FWADRL,A
	MOV   FWADRH,A
	MOV   A,R4
	MOV   FWID,A
	LJMP  SOLIN53

;;;;;;;;;;;;;;;;;;;;;;;;

SOLIN24:MOV   DPTR,#RSINBUF
	MOVX  A,@DPTR
	CJNE  A,#':',SOLIN26
	MOV   R4,#00H
	INC   DPTR
	LCALL GETBYT             ;data size
	JC    SOLIN27            ;if error
	XCH   A,R4
	ADD   A,R4
	XCH   A,R4
	JZ    SOLIN26A
	CJNE  A,#10H,SOLIN27
	MOV   R0,#TEMPBIN
	INC   R0                 ;space fo last addr
	INC   R0
	MOV   A,R0               ;begin: ah,al,data(16)
	MOV   R1,A               ;copy begin addr to R1
	LCALL GETBYT             ;ADDR high
	JC    SOLIN27
	XCH   A,R4               ;sum
	ADD   A,R4
	XCH   A,R4
	MOV   @R0,A
	INC   R0
	LCALL GETBYT             ;ADDR low
	JC    SOLIN27
	XCH   A,R4               ;sum
	ADD   A,R4
	XCH   A,R4
	MOV   @R0,A
	INC   R0
	LCALL GETBYT             ;record type = 0
	JC    SOLIN27
	XCH   A,R4               ;sum
	ADD   A,R4
	XCH   A,R4
	JNZ   SOLIN27
	MOV   R5,#10H
SOLIN25:LCALL GETBYT             ;data
	JC    SOLIN27
	XCH   A,R4               ;sum
	ADD   A,R4
	XCH   A,R4
	MOV   @R0,A
	INC   R0
	DJNZ  R5,SOLIN25
	LCALL GETBYT             ;checksum
	JC    SOLIN27
	ADD   A,R4               ;=0
	JNZ   SOLIN27
	MOV   A,@R1
	MOV   R4,A
	INC   R1
	CJNE  A,#18H,$+3
	JNC   SOLIN27
	CJNE  A,#17H,$+3
	JC    SOLIN28
	MOV   A,@R1
	CJNE  A,#0F1H,$+3
	JNC   SOLIN27		
	SJMP  SOLIN28
SOLIN26:
	LJMP  SOLIN34
SOLIN26A:
	MOV   DPL,#00H
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
	JNZ   SOLIN27
	CLR   A
	MOVC  A,@A+DPTR
	XRL   A,R1
	JNZ   SOLIN27
	LJMP  SOLIN33A
SOLIN27:MOV   DPS,#01H
	MOV   DPTR,#NRPRFX       ;send nrf1/
	MOV   DPS,#00H
	MOV   DPTR,#RSOUTBUF
	LCALL MOVPDAT
	MOV   DPS,#01H
	MOV   DPTR,#NRERTH       ;:Error
	MOV   DPS,#00H
	LCALL MOVPDAT
	LJMP  SOLIN72	         ;goto close&send line

SOLIN28:MOV   A,@R1
	INC   R1                 ;R1->data
	MOV   R5,A               ;R4:R5 - start FW ADDR
	ORL   A,R4
	JNZ   SOLIN30
	MOV   A,#NRFWOFS         ;if start ADDR=0
	CLR   C                  ;erase all pages
	RRC   A
	MOV   R3,A	
	MOV   A,#NAMTABH
	CLR   C
	RRC   A
	CLR   C
	SUBB  A,R3
	MOV   R2,A               ;number of pages
	MOV   A,R3
	CLR   IEN0.7
	SETB  FSR.5
SOLIN29:MOV   FCR,A              ;erase all pages
	INC   A
	DJNZ  R2,SOLIN29
	CLR   FSR.5
	SETB  IEN0.7
SOLIN30:MOV   A,R1
	MOV   R0,A
	MOV   A,R4
	ADD   A,#NRFWOFS
	MOV   R4,A
	MOV   DPH,A
	MOV   A,R5
	MOV   DPL,A
	MOV   R2,#10H
	CLR   IEN0.7
	SETB  FSR.5
SOLIN31:MOV   A,@R1
	INC   R1
	ORL   PCON,#10H
	MOVX  @DPTR,A
	ANL   PCON,#0EFH
	INC   DPTR
	DJNZ  R2,SOLIN31
	CLR   FSR.5
	SETB  IEN0.7
	MOV   A,R4
	MOV   DPH,A
	MOV   A,R5
	MOV   DPL,A
	MOV   R2,#10H
	CLR   A
	MOV   R3,A
SOLIN32:CLR   A
	MOVC  A,@A+DPTR					
	MOV   R1,A
	INC   DPTR
	MOV   A,@R0
	INC   R0
	XRL   A,R1
	JZ    SOLIN33
	INC   R3
SOLIN33:DJNZ  R2,SOLIN32
	MOV   A,R3
	JNZ   SOLIN27
SOLIN33A:
	MOV   DPS,#01H
	MOV   DPTR,#NRPRFX       ;send nrf1/
	MOV   DPS,#00H
	MOV   DPTR,#RSOUTBUF
	LCALL MOVPDAT
	MOV   DPS,#01H
	MOV   DPTR,#NROKTH       ;Ok
	MOV   DPS,#00H
	LCALL MOVPDAT
	LJMP  SOLIN72	         ;goto close&send line

SOLIN34:MOV   DPS,#01H
	MOV   DPTR,#NRRSTX
	MOV   DPS,#00H
	MOV   DPTR,#RSINBUF
	LCALL STPCOM             ;nrfresp?
	JNZ   SOLIN38
        LJMP  SOLIN53

SOLIN35:LCALL STPRRS
	MOV   A,#20H             ;space
	MOVX  @DPTR,A
	INC   DPTR
	MOV   DPS,#01H
	MOV   DPTR,#NROKTX       ;Ok
	MOV   DPS,#00H
	LCALL MOVPDAT
	LJMP  SOLIN72	         ;goto close&send line

SOLIN36:LCALL STPRRS
	MOV   A,#20H             ;space
	MOVX  @DPTR,A
	INC   DPTR
	MOV   DPS,#01H
	MOV   DPTR,#NRERTX       ;Error
	MOV   DPS,#00H
	LCALL MOVPDAT
	LJMP  SOLIN72	         ;goto close&send line

SOLIN38:CLR   A
	MOV   RSTO,A             ;clear RS timeout
	MOV   DPTR,#RSINBUF      ;scan NAMTAB 
	MOV   R3,DPH
	MOV   R4,DPL	
	MOV   R2,#0FFH
SOLIN39:INC   R2                 ;next record
	MOV   A,R2               ;00ddddii
	CJNE  A,#40H,SOLIN40     ;64 records 16 dev * 4 
        LJMP  SOLIN53
SOLIN40:SWAP  A                  ;ddii00dd
	RL    A                  ;dii00ddd
	MOV   R0,A
	ANL   A,#07H
	ADD   A,#NAMTABH
	MOV   DPH1,A
	MOV   A,R0
	ANL   A,#0E0H            ;begin of record
	MOV   DPL1,A
	MOV   DPH,R3
	MOV   DPL,R4
	MOV   A,#NAMSZ           ;max name size in record
	LCALL STPCOMD
	JNZ   SOLIN39
	MOV   R4,A
	MOV   MPAGE,R2
SOLIN41:MOVX  A,@DPTR
	CJNE  A,#20H,SOLIN42
	INC   DPTR
	SJMP  SOLIN41
SOLIN42:CJNE  A,#2DH,$+3         ;if digit
	JC    SOLIN43            ;43
	CJNE  A,#3AH,$+3
	JNC   SOLIN48            ;43
	MOV   R1,#0F0H
	MOV   R0,#TEMPBIN
	LCALL DECBIN             ;convert to binary
	JNZ   SOLIN43            ;43
	MOV   R4,#0FFH
SOLIN43:MOV   A,MPAGE            ;if no error
	MOV   R3,A
	RR    A
	ANL   A,#1EH
	ADD   A,#RFBITM
	MOV   R0,A               ;R0 point to bitmass
	MOV   A,R3
	ANL   A,#03H
	ADD   A,#04H
	MOV   R2,A
	MOV   A,#01H
SOLIN44:RL    A
	DJNZ  R2,SOLIN44
	MOV   R2,A               ;R2 mask
	MOV   A,@R0
	MOV   R1,A               	
	ANL   A,R2
	JZ    SOLIN46            ;rs bit set?
	MOV   A,R2               ;yes, reset bit
	CPL   A
	ANL   A,R1
	MOV   @R0,A	
SOLIN45:LJMP  SOLIN53            ;no response
SOLIN46:MOV   A,R4
	JZ    SOLIN45
	MOV   A,R2               ;if bit not set
	SWAP  A                  ;command       
	ORL   A,R1               ;yes, prepare rf bit 
	MOV   R4,A               ;R4 mask
	MOV   A,R0
	MOV   R5,A               ;R5 pointer in bitm
	MOV   A,R3               ;R0 point to data
	RL    A
	RL    A
	MOV   R0,A
	MOV   MPAGE,#00H
	MOV   R1,#TEMPBIN        ;copy binary
	MOV   R2,#04H            ;to data
SOLIN47:MOV   A,@R1
	MOVX  @R0,A
	INC   R0
	INC   R1
	DJNZ  R2,SOLIN47			
	MOV   A,R5
	MOV   R0,A
	MOV   A,R4
	MOV   @R0,A              ;set rf bit
	LJMP  SOLIN53
SOLIN48:MOV   R3,DPH             ;if no digit
	MOV   R4,DPL	         ;verify if true
	MOV   DPS,#01H           ;or false
	MOV   DPTR,#NRTRTX
	MOV   DPS,#00H
	LCALL STPCOM             ;true?
	MOV   R1,#01H
	MOV   R2,#0FFH
	JZ    SOLIN50            ;store value ff,ff,ff,1
	MOV   DPS,#01H
	MOV   DPTR,#NRONTX
	MOV   DPS,#00H
        MOV   DPH,R3
	MOV   DPL,R4
	LCALL STPCOM             ;on?
	MOV   R1,#01H
	MOV   R2,#0FEH
	JZ    SOLIN50            ;store value ff,ff,fe,1
	MOV   DPS,#01H
	MOV   DPTR,#NROFTX
	MOV   DPS,#00H
        MOV   DPH,R3
	MOV   DPL,R4
	LCALL STPCOM             ;off
	MOV   R1,#00H
	MOV   R2,#0FEH
	JZ    SOLIN50            ;store value ff,ff,fe,0
	MOV   DPS,#01H
	MOV   DPTR,#NRFSTX
	MOV   DPS,#00H
        MOV   DPH,R3
	MOV   DPL,R4
	LCALL STPCOM             ;false?
	MOV   R1,#00H            ;store value ff,ff,ff,0
	MOV   R2,#0FFH
       	JZ    SOLIN50
	MOV   DPS,#01H
	MOV   DPTR,#NRNRFTX
	MOV   DPS,#00H
        MOV   DPH,R3
	MOV   DPL,R4
	LCALL STPCOM             ;NoRFL?
       	JZ    SOLIN52
SOLIN49:MOV   R4,#00H            ;reset rf bit if response
	LJMP  SOLIN43
SOLIN50:MOV   R4,#0FFH
	MOV   R0,#TEMPBIN
	MOV   A,R1
	MOV   @R0,A
	INC   R0
	CLR   A
;	MOV   A,R2               ;uncomment enables true-false response
	MOV   @R0,A
	INC   R0
	CLR   A
;	CPL   A                  ;uncomment enables true-false response
	MOV   @R0,A
	INC   R0
	MOV   @R0,A
        LJMP  SOLIN43
SOLIN51:LJMP  SOLINE             ;no response
SOLIN52:MOV   A,MPAGE            ;if no error
	RR    A
	ANL   A,#1EH
	ADD   A,#RFBITM
	MOV   R0,A               ;R0 point to bitmass
	INC   R0                 ;timeout
	MOV   A,@R0
	ANL   A,#0FH             ;in acc timeout counter
	XRL   A,#0FH             ;if norfl exit
	JZ    SOLIN49
	DEC   R0
	MOV   A,@R0
	ORL   A,#0F0H            ;set all RF command bits
	MOV   @R0,A
SOLIN53:MOV   A,RSTMR
	CJNE  A,#0BFH,$+3
	JNC   SOLIN51
	JZ    SOLIN53A
	MOV   A,RSTO
	CJNE  A,#04H,$+3
	JNC   SOLIN51
SOLIN53A:
	MOV   R6,#02H            ;2 loops
	MOV   A,RSID             ;current rfid
	ANL   A,#3FH             ;64
	MOV   RSID,A             ;correct if needed
SOLIN54:MOV   R3,A
	RR    A
	ANL   A,#1EH
	ADD   A,#RFBITM
	MOV   R0,A               ;R0 point to bitmass
	MOV   A,R3
	ANL   A,#03H
	ADD   A,#04H
	MOV   R2,A
	MOV   A,#01H
SOLIN55:RL    A
	DJNZ  R2,SOLIN55
	MOV   R2,A               ;R2 mask
	MOV   A,R6               ;if second loop
	DEC   A                  ;skip send flag
	JZ    SOLIN56
	MOV   A,@R0              ;load bits
	MOV   R1,A               	
	ANL   A,R2
	JZ    SOLIN57            ;rs bit set?
SOLIN56:MOV   A,R3
	SWAP  A                  ;ddii00dd
	RL    A                  ;dii00ddd
	MOV   R4,A
	ANL   A,#07H
	ADD   A,#NAMTABH
	MOV   DPH,A
	XCH   A,R4
	ANL   A,#0E0H            ;begin of record
	MOV   DPL,A
	MOV   R5,A
	CLR   A
	MOVC  A,@A+DPTR
	JZ    SOLIN57
	INC   A
	JZ    SOLIN57
	LJMP  SOLIN64            
SOLIN57:MOV   A,R3               ;if bit not set for first
	INC   A                  ;or noname
	ANL   A,#3FH             ;next rsid
	CJNE  A,RSID,SOLIN54
	MOV   A,RSTMR
	JNZ   SOLIN60
SOLIN58:MOV   A,RSID
	DJNZ  R6,SOLIN54
	MOV   DPS,#01H
	MOV   DPTR,#NRPRFX       ;m send nrf1/nrfcomm
	MOV   DPS,#00H
	MOV   DPTR,#RSOUTBUF
	LCALL MOVPDAT
	MOV   DPS,#01H
	MOV   DPTR,#NRCMTX
	MOV   DPS,#00H
	LCALL MOVPDAT
	MOV   A,#20H             ;+space
	MOVX  @DPTR,A
	INC   DPTR
	MOV   A,#2EH             ;+space
	MOVX  @DPTR,A
	INC   DPTR
	MOV   A,#0DH             ;+0ah
	MOVX  @DPTR,A
	INC   DPTR
	MOV   A,#0AH             ;+0ah
	MOVX  @DPTR,A
	INC   DPTR
	CLR   A
	MOVX  @DPTR,A
	MOV   DPTR,#RSOUTBUF
	MOVX  A,@DPTR
	MOV   15H,DPL
	MOV   14H,DPH
	MOV   S0BUF,A
	MOV   A,#0F0H
	MOV   RSTMR,A	
SOLIN59:LJMP  SOLINE	
SOLIN60:MOV   A,FWID
	JZ    SOLIN59
	LCALL STPRRS
	MOV   A,#20H             ;space
	MOVX  @DPTR,A
	INC   DPTR
	MOV   DPS,#01H
	MOV   DPTR,#NRFWLD       ;store ldfw
	MOV   DPS,#00H
	LCALL MOVPDAT
	MOV   A,#5FH             ;_ space
	MOVX  @DPTR,A
	INC   DPTR
	MOV   A,#TBINDEC
	MOV   R0,A
	MOV   R1,A
	MOV   A,FWID
	ANL   A,#1FH
	MOV   @R1,A
	INC   R1
	CLR   A
	MOV   @R1,A
	INC   R1
	MOV   @R1,A
	INC   R1
	MOV   @R1,A
	MOV   R1,#0F0H
	LCALL BINDEC
	MOV   A,#5FH             ;_ space
	MOVX  @DPTR,A
	INC   DPTR
	MOV   A,FWID
	JNB   ACC.7,SOLIN61
	MOV   DPS,#01H
	MOV   DPTR,#NRERTX       ;store Error
	MOV   DPS,#00H
	LCALL MOVPDAT
	SJMP  SOLIN62
SOLIN61:MOV   A,FWADRH
	CJNE  A,#18H,SOLIN63
	MOV   DPS,#01H
	MOV   DPTR,#NROKTX       ;store Ok
	MOV   DPS,#00H
	LCALL MOVPDAT
SOLIN62:CLR   A
	MOV   FWID,A
	MOV   FWADRL,A
	MOV   FWADRH,A
	AJMP  SOLIN72
SOLIN63:MOV   A,#TBINDEC
	MOV   R0,A
	MOV   R1,A
	MOV   A,FWADRL
	MOV   @R1,A
	INC   R1
	MOV   A,FWADRH
	MOV   @R1,A
	INC   R1
	CLR   A
	MOV   @R1,A
	INC   R1
	MOV   @R1,A
	MOV   R1,#0F0H
	LCALL BINDEC             ;convert
	MOV   DPS,#01H
	MOV   DPTR,#NRBTTX       ;store bytes
	MOV   DPS,#00H
	LCALL MOVPDAT
	AJMP SOLIN72

SOLIN64:MOV   A,RSTO
	INC   A
	JZ    SOLIN64A
	MOV   RSTO,A             ;inc RSTO
SOLIN64A:
	MOV   A,RSID             ;if curr rsid=prev rsid
	XRL   A,R3
	ANL   A,#3FH
	JZ    SOLIN65            ;update rsid anyway
	MOV   A,R6               ;if no & first loop
	DEC   A                  ;skip rsid update 
	JNZ   SOLIN66
SOLIN65:MOV   A,R3               ;set next RSID
	INC   A
	ANL   A,#3FH
	MOV   RSID,A
SOLIN66:MOV   A,R2               ;set RS bit
	ORL   A,R1
	MOV   @R0,A
	MOV   DPS,#01H
	MOV   DPTR,#NRPRFX       ;store nrf1/
	MOV   DPS,#00H
	MOV   DPTR,#RSOUTBUF
	LCALL MOVPDAT
	MOV   DPH1,R4            ;store name
	MOV   DPL1,R5
	MOV   R4,#20H
SOLIN67:MOV   DPS,#01H
	CLR   A
	MOVC  A,@A+DPTR
	INC   DPTR
	MOV   DPS,#00H
	JZ    SOLIN68
	INC   A
	JZ    SOLIN68
	DEC   A
	MOVX  @DPTR,A
	INC   DPTR
	DJNZ  R4,SOLIN67
SOLIN68:MOV   A,#20H             ;store space
	MOVX  @DPTR,A
	INC   DPTR
	MOV   A,R3
	RR    A
	ANL   A,#1EH
	ADD   A,#RFBITM
	INC   A
	MOV   R0,A               ;R0 to RFLTO
	MOV   A,@R0
	ANL   A,#0FH
	CJNE  A,#0FH,SOLIN69
	MOV   DPS,#01H
	MOV   DPTR,#NRNRFTX      ;store NoRFL
	MOV   DPS,#00H
	LCALL MOVPDAT
	SJMP  SOLIN72
SOLIN69:MOV   MPAGE,#00H	
	MOV   A,R3
	ADD   A,ACC
	ADD   A,ACC
	MOV   R0,A
	ADD   A,#03H             ;before digit conversion
	MOV   R1,A               ;check
	MOVX  A,@R1              ;if received
	CJNE  A,#0FFH,SOLIN71    ;frue ff,ff,ff,01
	DEC   R1                 ;or false ff,ff,ff,00
	MOVX  A,@R1              ;or off ff,ff,fe,00
	CJNE  A,#0FFH,SOLIN71    ;or on ff,ff,fe,01
	DEC   R1
	MOVX  A,@R1
	DEC   R1
	CJNE  A,#0FEH,SOLIN69A
	MOV   DPS,#01H
	MOV   DPTR,#NROFTX       ;store off text
	MOVX  A,@R1
	JZ    SOLIN70	
	MOV   DPTR,#NRONTX       ;store on  text
	SJMP  SOLIN70
SOLIN69A:
	JC    SOLIN71
	MOV   DPS,#01H
	MOV   DPTR,#NRFSTX       ;store false text
	MOVX  A,@R1
	JZ    SOLIN70	
	MOV   DPTR,#NRTRTX       ;store true  text
SOLIN70:MOV   DPS,#00H
	LCALL MOVPDAT
	SJMP  SOLIN72
SOLIN71:MOV   R1,#00H
	LCALL BINDEC             ;convert
SOLIN72:MOV   A,#0DH
	MOVX  @DPTR,A
	INC   DPTR
	MOV   A,#0AH
	MOVX  @DPTR,A
	INC   DPTR
	CLR   A
	MOVX  @DPTR,A
	MOV   DPTR,#RSOUTBUF
	MOVX  A,@DPTR
	MOV   15H,DPL
	MOV   14H,DPH
	MOV   S0BUF,A
	MOV   A,#0C0H
	MOV   RSTMR,A	
SOLINE: POP   MPAGE
	RET

;******************************************************;
; GETBYT read byte @DPTR A-byte, C=1 - error
; used R2,R3
;******************************************************;
GETBYT: MOV   R2,#00H
	MOV   R3,#02H
GETBYT1:MOVX  A,@DPTR
	CLR   C
	SUBB  A,#30H
	JC    GETBYT3
	CJNE  A,#0AH,$+3
	JC    GETBYT2		
	ANL   A,#0DFH
	SUBB  A,#07H
	CJNE  A,#10H,$+3
	JNC   GETBYT3
GETBYT2:INC   DPTR
	XCH   A,R2
	SWAP  A
	ORL   A,R2
	MOV   R2,A
	DJNZ  R3,GETBYT1
	MOV   A,R2
	CLR   C
	SJMP  GETBYT4
GETBYT3:CLR   A
	SETB  C
GETBYT4:RET

;******************************************************;
; SKPSPC skip spaces @DPTR
;******************************************************;
SKPSPC: MOVX  A,@DPTR
	CJNE  A,#20H,SKPSPC1
	INC   DPTR
	SJMP  SKPSPC
SKPSPC1:RET

;******************************************************;
;GTPAR  get unit & par numbers in A, A=0FF - error
;use A,R0-R3
;******************************************************;
GTPAR:  MOV   R1,DPH
	MOV   R2,DPL
	MOV   R3,#00H
GTPAR1:	MOVX  A,@DPTR
	CJNE  A,#20H,GTPAR2
	INC   DPTR
	SJMP  GTPAR1
GTPAR2: JC    GTPAR6
	CLR   C
	SUBB  A,#30H
	JC    GTPAR6
	CJNE  A,#0AH,$+3
	JNC   GTPAR6
	MOV   R0,A
	INC   DPTR
	MOVX  A,@DPTR
	CJNE  A,#20H,GTPAR3
	INC   R3
	SJMP  GTPAR4
GTPAR3:	JC    GTPAR6
	CLR   C
	SUBB  A,#30H
	JC    GTPAR6
	CJNE  A,#0AH,$+3
	JNC   GTPAR6
	XCH   A,R0
	MOV   B,#0AH
	MUL   AB
	ADD   A,R0
	MOV   R0,A
	MOV   A,B
	JNZ   GTPAR6
GTPAR4:	MOV   A,R0
	DEC   A
	CJNE  A,#10H,$+3
	JNC   GTPAR6
	RL    A
	RL    A
	MOV   R0,A
GTPAR5:	INC   DPTR
	MOVX  A,@DPTR
	CJNE  A,#20H,GTPAR7
	INC   R3
	SJMP  GTPAR5
GTPAR6: MOV   DPH,R1
	MOV   DPL,R2
	CLR   A
	CPL   A
	SJMP  GTPAR8
GTPAR7: CLR   C
	SUBB  A,#30H
	JC    GTPAR6
	JZ    GTPAR6
	CJNE  A,#05H,$+3
	JNC   GTPAR6
	XCH   A,R3
	JZ    GTPAR6
	XCH   A,R3
	INC   DPTR
	DEC   A
	ANL   A,#03H
	ORL   A,R0
GTPAR8: RET

;******************************************************;
;GTNUM  get unit & par numbers in A, A=0FF - error
;use A,R0-R2
;******************************************************;
GTNUM:  MOV   R1,DPH
	MOV   R2,DPL
GTNUM1:	MOVX  A,@DPTR
	CJNE  A,#20H,GTNUM2
	INC   DPTR
	SJMP  GTNUM1
GTNUM2: JC    GTNUM6
	CLR   C
	SUBB  A,#30H
	JC    GTNUM6
	CJNE  A,#0AH,$+3
	JNC   GTNUM6
	MOV   R0,A
	INC   DPTR
	MOVX  A,@DPTR
	CJNE  A,#20H,$+3
        JC    GTNUM4
	CLR   C
	SUBB  A,#30H
	JC    GTNUM6
	CJNE  A,#0AH,$+3
	JNC   GTNUM6
	XCH   A,R0
	MOV   B,#0AH
	MUL   AB
	ADD   A,R0
	MOV   R0,A
	MOV   A,B
	JNZ   GTNUM6
GTNUM4:	MOV   A,R0
	CJNE  A,#11H,$+3
	JC    GTNUM7
GTNUM6: MOV   DPH,R1
	MOV   DPL,R2
	CLR   A
	CPL   A
GTNUM7: RET

;******************************************************;
; STPCOM - COMPARE XRAM DPTR & PROM DPTR1
; used R0,R1
;******************************************************;
STPCOM: CLR   A
	CPL   A
STPCOMD:MOV   R1,A
STPCOM1:MOV   DPS,#00H           ;low DPTR - buf
	MOVX  A,@DPTR
	CJNE  A,#21H,$+3
	JC    STPCOM5            ;if <=20H - end of name in buf compare ok
	CJNE  A,#80H,$+3
	JNC   STPCOM2
	CJNE  A,#61H,$+3
	JC    STPCOM2
	ANL   A,#5FH
STPCOM2:MOV   R0,A
	MOV   DPS,#01H
	CLR   A
	MOVC  A,@A+DPTR          ;byte from name
	MOV   DPS,#00H
	JZ    STPCOM4            ;if 0 or 0ff - end of name
	INC   A                  ;then
	JZ    STPCOM4            ;next record to compare
	DEC   A
	CJNE  A,#80H,$+3
	JNC   STPCOM3
	CJNE  A,#61H,$+3
	JC    STPCOM3
	ANL   A,#5FH
STPCOM3:XRL   A,R0               ;compare
	JNZ   STPCOM4
	INC   DPTR               ;ok, next symbol
	MOV   DPS,#01H
	INC   DPTR
	MOV   DPS,#00H
	DJNZ  R1,STPCOM1         ;if end of record
STPCOM4:CLR   A
	CPL   A
	SJMP  STPCOM7            ;next
STPCOM5:MOV   DPS,#01H
	CLR   A
	MOVC  A,@A+DPTR
	MOV   DPS,#00H
        JZ    STPCOM6            ;0 or 0ff - end of name
	INC   A
	JNZ   STPCOM4
STPCOM6:CLR   A
STPCOM7:RET

;******************************************************;
; MOVPDAT - copy from PROM DPTR1 to XRAM DPTR
; used R0,R1
;******************************************************;
MOVPDAT:MOV   DPS,#01H
	CLR   A
	MOVC  A,@A+DPTR
	INC   DPTR
	MOV   DPS,#00H
	JZ    MOVPDAT1
	INC   A
	JZ    MOVPDAT1
	DEC   A
	MOVX  @DPTR,A
	INC   DPTR
	SJMP  MOVPDAT
MOVPDAT1:
;	MOV   A,#0AH
;	MOVX  @DPTR,A
	RET	

;******************************************************;
; STPRRS store prefix nrf1/nrfresp @DPTR=#RSOUTBUF
;******************************************************;
STPRRS:	MOV   DPS,#01H
	MOV   DPTR,#NRPRFX       ;m send nrf1/
	MOV   DPS,#00H
	MOV   DPTR,#RSOUTBUF
	LCALL MOVPDAT
	MOV   DPS,#01H
	MOV   DPTR,#NRRSTX       ;nrfresp
	MOV   DPS,#00H
	LCALL MOVPDAT
	RET


;******************************************************;
; RFSEND fill outbuf for RFID & prepare to send 
; A<>0 - busy
;******************************************************;
RFSEND: PUSH  MPAGE
	MOV   A,01FH             ;if sending
	ORL   A,01EH             ;or receiving 
	JZ    RFSEND1            ;exit
	AJMP  RFSENDX
RFSEND1:MOV   A,FWID             ;if =0
	JZ    RFSEND2            ;no FW sended
	DEC   A
	MOV   R0,A               ;check if FW sending to current RFID
	MOV   A,RFID
	ANL   A,#0FH
	XRL   A,R0
	JZ    RFSEND6
RFSEND2:MOV   DPTR,#RFTXBUF      ;fill RFTX buf
	MOV   A,RFID             ;if data sending
	ANL   A,#0FH             ;+0-RF id
	MOVX  @DPTR,A
	INC   DPTR
	MOV   A,RFID
	ANL   A,#0FH
	RL    A
	ADD   A,#RFBITM
	MOV   R0,A
	MOV   A,@R0
	ANL   A,#0FH
	MOVX  @DPTR,A            ;+1 modify bits
	INC   R0
	MOV   A,@R0              ;RFLTO counter
	MOV   R1,A
	ANL   A,#0FH
	XRL   A,#0FH
	JZ    RFSEND3            ;0F - norfl
	MOV   A,R1               ;increment counter
	INC   A
	MOV   @R0,A
	ANL   A,#0FH
	XRL   A,#0FH
	JNZ   RFSEND4            ;new value no timeout, skip
	DEC   R0                 ;new value timeout
	MOV   A,@R0              ;set bits
	ORL   A,#0F0H            ;report to server
	MOV   @R0,A
	SJMP  RFSEND4
RFSEND3:                         ;if norfl clear client command bits
	DEC   R0
	MOV   A,@R0
	ANL   A,#0F0H
	MOV   @R0,A
RFSEND4:
	INC   DPTR
	CLR   A
	CPL   A
	MOVX  @DPTR,A            ;+2 0ffH
	INC   DPTR
	MOVX  @DPTR,A            ;+3 0ffH
	INC   DPTR
	MOV   A,RFID
	ANL   A,#0FH
	SWAP  A
	MOV   MPAGE,#00H	
	MOV   R0,A
	MOV   R1,#10H            ;+4 4*4 data
RFSEND5:
	MOVX  A,@R0
	MOVX  @DPTR,A
	INC   R0
	INC   DPTR
	DJNZ  R1,RFSEND5	
	CLR   A
	CPL   A
	MOVX  @DPTR,A            ;+20 0ffH
	INC   DPTR
	MOVX  @DPTR,A            ;+21 0ffH
	INC   DPTR
	MOVX  @DPTR,A            ;+22 0ff
	INC   DPTR
	MOVX  @DPTR,A            ;+23 0ff
	SJMP  RFSEND12

RFSEND6:MOV   DPTR,#RFTXBUF      ;fill RFTX buf
	MOV   A,RFID             ;if data sending
	ANL   A,#0FH             ;+0-RF id
	ORL   A,#80H             ;comm bit in addr
	MOVX  @DPTR,A
	INC   DPTR
	MOV   A,RFID
	ANL   A,#0FH
	RL    A
	ADD   A,#RFBITM
	MOV   R0,A
	INC   R0
	MOV   A,@R0
	MOV   R1,A
	ANL   A,#0FH             ;check timeout
	XRL   A,#0FH
	JNZ   RFSEND8
	MOV   A,FWID             ;if expired or other error
	ORL   A,#80H             ;set error
	MOV   FWID,A
	SJMP  RFSEND2            ;goto data send
RFSEND8:MOV   A,R1
	INC   A                  ;increment timeout
	MOV   @R0,A
	CLR   A
	MOVX  @DPTR,A            ;+1 modify bits =0
	INC   DPTR
	MOV   A,FWADRL
	ANL   A,#0F0H            ;align 16 bytes
	MOV   FWADRL,A
	MOV   DPL1,A
	MOVX  @DPTR,A            ;+2 addr low
	MOV   R2,DPL
	MOV   R3,DPH
	INC   DPTR
	MOV   A,FWADRH
	CJNE  A,#18H,$+3
	JC    RFSEND9
	AJMP  RFSEND2
RFSEND9:MOVX  @DPTR,A            ;+3 addr high
	INC   DPTR
	ADD   A,#NRFWOFS
	MOV   DPH1,A
	MOV   R1,#10H            ;+4 16 bytes data
RFSEND10:
	MOV   DPS,#01H
	CLR   A
	MOVC  A,@A+DPTR
	INC   DPTR
	MOV   DPS,#00H
	MOVX  @DPTR,A	
	INC   DPTR
	DJNZ  R1,RFSEND10		
	MOV   DPL1,R2
	MOV   DPH1,R3
	MOV   DPS,#01H
	MOV   R2,#12H
	LCALL CLCRC
	MOV   DPS,#00H
	MOV   A,R0
	MOVX  @DPTR,A            ;+20 crcl
	INC   DPTR
	MOV   A,R1
	MOVX  @DPTR,A            ;+21 crch
	INC   DPTR
	CLR   A
	CPL   A
	MOVX  @DPTR,A            ;+22 0ff
	INC   DPTR
	MOVX  @DPTR,A            ;+23 0ff
RFSEND12:
	MOV   1EH,#00H           ;RX busy flag
	MOV   1FH,#01H           ;TX busy flag
        MOV   TH0,#0             ;#HI_T250U
        MOV   TL0,#0             ;#LO_T250U
        SETB  TCON.4             ;start timer0
	SETB  IEN0.1             ;en timer0
	CLR   A
RFSENDX:POP   MPAGE
	RET		

;******************************************************;
; RFREAD fill outbuf for RFID & prepare to send 
; A<>0 - busy
;******************************************************;
RFREAD:	PUSH  MPAGE
	MOV   A,1FH              ;if sending 
	JZ    RFREAD3
	INC   A                  ;if sended
	JZ    RFREAD3            
RFREAD1:
	AJMP  RFREAD25           ;exit
RFREAD2:
	AJMP  RFREAD26           ;if ldfw
RFREAD3:
	MOV   A,1EH              ;if no readed
	JZ    RFREAD1            ;exit
	MOV   DPTR,#RFRXBUF
	MOVX  A,@DPTR            ;RX_ADDR
	INC   DPTR               ;+1
	JB    ACC.7,RFREAD2      ;bit 7 set - command frame
	CJNE  A,#10H,$+3         ;if reset - data frame
	JNC   RFREAD1            ;if invalid RFID ignore frame
	ANL   A,#0FH             ;only 32 
	MOV   R0,A
	RL    A
	ADD   A,#RFBITM
	MOV   R1,A               ;R1 - point to bitm
	MOV   A,R0
	SWAP  A
	MOV   MPAGE,#00H	
	MOV   R0,A               ;R0 - point to data
	MOVX  A,@DPTR
	INC   DPTR               ;+2
	MOV   R4,A               ;rxbitm???
	INC   DPTR               ;+3
	INC   DPTR               ;+4
	MOV   R3,#4              ;4 bytes data1
	MOV   A,@R1              ;if snd rf bit set
	JNB   ACC.0,RFREAD10      
	MOV   A,R4               ;rec rf bit must set
	JB    ACC.0,RFREAD10      
RFREAD9:	                 ;if no ignore readed value
	INC   R0
	INC   DPTR
	DJNZ  R3,RFREAD9
	SJMP  RFREAD12
RFREAD10:
	MOVX  A,@R0
	MOV   R2,A
	MOVX  A,@DPTR
	MOVX  @R0,A
	XRL   A,R2
	JZ    RFREAD11
	MOV   A,@R1
	SETB  ACC.4
	MOV   @R1,A
RFREAD11:	
	INC   R0
	INC   DPTR
	DJNZ  R3,RFREAD10
	MOV   A,@R1
	CLR   ACC.0
	MOV   @R1,A
RFREAD12:
	MOV   R3,#4              ;4 bytes data2
	MOV   A,@R1              ;if snd rf bit set
	JNB   ACC.1,RFREAD14      
	MOV   A,R4               ;rec rf bit must set
	JB    ACC.1,RFREAD14      
RFREAD13:	                 ;if no ignore readed value
	INC   R0
	INC   DPTR
	DJNZ  R3,RFREAD13
	SJMP  RFREAD16
RFREAD14:
	MOVX  A,@R0
	MOV   R2,A
	MOVX  A,@DPTR
	MOVX  @R0,A
	XRL   A,R2
	JZ    RFREAD15
	MOV   A,@R1
	SETB  ACC.5
	MOV   @R1,A
RFREAD15:	
	INC   R0
	INC   DPTR
	DJNZ  R3,RFREAD14
	MOV   A,@R1
	CLR   ACC.1
	MOV   @R1,A
RFREAD16:
	MOV   R3,#4              ;4 bytes data3
	MOV   A,@R1              ;if snd rf bit set
	JNB   ACC.2,RFREAD18      
	MOV   A,R4               ;rec rf bit must set
	JB    ACC.2,RFREAD18      
RFREAD17:	                 ;if no ignore readed value
	INC   R0
	INC   DPTR
	DJNZ  R3,RFREAD17
	SJMP  RFREAD20
RFREAD18:
	MOVX  A,@R0
	MOV   R2,A
	MOVX  A,@DPTR
	MOVX  @R0,A
	XRL   A,R2
	JZ    RFREAD19
	MOV   A,@R1
	SETB  ACC.6
	MOV   @R1,A
RFREAD19:	
	INC   R0
	INC   DPTR
	DJNZ  R3,RFREAD18
	MOV   A,@R1
	CLR   ACC.2
	MOV   @R1,A
RFREAD20:
	MOV   R3,#4              ;4 bytes data4
	MOV   A,@R1              ;if snd rf bit set
	JNB   ACC.3,RFREAD22      
	MOV   A,R4               ;rec rf bit must set
	JB    ACC.3,RFREAD22      
RFREAD21:	                 ;if no ignore readed value
	INC   R0
	INC   DPTR
	DJNZ  R3,RFREAD21
	SJMP  RFREAD24
RFREAD22:
	MOVX  A,@R0
	MOV   R2,A
	MOVX  A,@DPTR
	MOVX  @R0,A
	XRL   A,R2
	JZ    RFREAD23
	MOV   A,@R1
	SETB  ACC.7
	MOV   @R1,A
RFREAD23:	
	INC   R0
	INC   DPTR
	DJNZ  R3,RFREAD22
	MOV   A,@R1
	CLR   ACC.3
	MOV   @R1,A
RFREAD24:
	MOV   A,R4
	SWAP  A
	ANL   A,#0F0H
	MOV   R4,A
	MOV   A,@R1
	ORL   A,R4
	MOV   @R1,A
	INC   R1
	MOV   A,@R1
	ANL   A,#0F0H
	XCH   A,@R1              ;clear RFLTO counter
	MOV   1EH,#00H
	ANL   A,#0FH             ;in acc old counter
	CJNE  A,#0FH,RFREAD25    ;if old norfl
	DEC   R1
	MOV   A,@R1
	ORL   A,#0F0H            ;set all RF command bits
	MOV   @R1,A
RFREAD25:
	SJMP  RFREADE
RFREAD26:
	ANL   A,#7FH
	CJNE  A,#10H,$+3
	JNC   RFREAD25
	ANL   A,#0FH             ;only 32 
	MOV   R0,A
	RL    A
	ADD   A,#RFBITM
	MOV   R1,A               ;R1 - point to bitm
	MOV   A,FWID
	JB    ACC.7,RFREAD30
	JZ    RFREAD30
	DEC   A
	CJNE  A,#10H,$+3
	JNC   RFREAD30
	XRL   A,R0               ;check if RFID=FWID
	JNZ   RFREAD30           ;clr timeout+exit
	MOVX  A,@DPTR
	INC   DPTR               ;+2
	JNZ   RFREAD29           ;=0 - ACK, else NACK
	MOVX  A,@DPTR
	MOV   R3,A
	INC   DPTR               ;+3
	MOVX  A,@DPTR
	MOV   R2,A
	MOV   R4,#02H
RFREAD27:
	MOV   A,R3
	XRL   A,FWADRL
	JNZ   RFREAD28
	MOV   A,R2
	XRL   A,FWADRH
RFREAD28:		
	JZ    RFREAD30		
	MOV   A,FWADRL
	ADD   A,#10H
	MOV   FWADRL,A
	MOV   A,FWADRH
	ADDC  A,#00H
	MOV   FWADRH,A
	DJNZ  R4,RFREAD27
RFREAD29:
	MOV   A,FWID
	ORL   A,#80H
	MOV   FWID,A
RFREAD30:
	INC   R1
	MOV   A,@R1
	ANL   A,#0F0H
	MOV   @R1,A              ;clear RFLTO counter
	MOV   1EH,#00H
RFREADE:	
	POP   MPAGE
	RET		

;******************************************************;
; RFNXTID select next RFID
;******************************************************;
RFNXID: PUSH  MPAGE
        MOV   A,1FH              ;if sending exit
	INC   A
	JNZ   RFNXIDE
	MOV   A,1EH              ;if readed frame present 
	JNZ   RFNXIDE            ;exit
	MOV   A,RFID             ;current rfid
	ANL   A,#0FH             ;16
	MOV   RFID,A             ;correct if needed
	MOV   R0,A
RFNXID1:                         ;searching for command not send yet
	MOV   A,R0               ;next id
	INC   A
	ANL   A,#0FH
	MOV   R0,A
	RL    A
	ADD   A,#RFBITM
	MOV   R1,A               ;R1 - point to bitm
	MOV   A,@R1              ;read next
	ANL   A,#0FH
	JZ    RFNXID2            ;no command
	INC   R1
	MOV   A,@R1              ;RF link live?
	ANL   A,#0FH
	CJNE  A,#02H,$+3         ;1 time lost ignored
	JC    RFNXID8            ;yes command found
RFNXID2:
	MOV   A,R0
	CJNE  A,RFID,RFNXID1     ;if no command found
	MOV   A,FWID
	JZ    RFNXID3
	JB    ACC.7,RFNXID3
	DEC   A
	ANL   A,#0FH
	MOV   R0,A
	SJMP  RFNXID8
RFNXID3:MOV   A,PRFID
RFNXID4:INC   A                  ;next rfid for polling
	ANL   A,#0FH
	MOV   R0,A
	RR    A                  ;point to name tables
	MOV   DPH,A
	ANL   A,#080H
	MOV   DPL,A
	MOV   A,DPH
	ANL   A,#07H
	ADD   A,#NAMTABH
	MOV   DPH,A
	MOV   R1,#04H
RFNXID5:CLR   A                  ;check if at least
	MOVC  A,@A+DPTR          ;one name exist 
	JZ    RFNXID6
	INC   A
	JNZ   RFNXID7
RFNXID6:MOV   A,DPL
	ADD   A,#20H
	MOV   DPL,A
	MOV   A,DPH
	ADDC  A,#00H
	MOV   DPH,A
	DJNZ  R1,RFNXID5
	MOV   A,R0
	CJNE  A,PRFID,RFNXID4
RFNXID7:MOV   A,R0
	MOV   PRFID,A
RFNXID8:                         ;switch rfid
	MOV   RFID,R0
	LCALL STRFADR
	MOV   1FH,#00
RFNXIDE:	
	POP   MPAGE
	RET		


;******************************************************;
; DECBIN in - @DPTR, out: @(R1:R0), A=0 Ok, A<>0 Error
; used A,B,R0-R7
;******************************************************;
DECBIN: PUSH  MPAGE
	MOV   MPAGE,R1
	CLR   A
	MOV   R3,A
	MOV   R2,#7FH
DECBIN1:MOVX  A,@DPTR
	CJNE  A,#20H,DECBIN2     ;remove leader spaces
	INC   DPTR
	SJMP  DECBIN1
DECBIN2:CJNE  A,#2DH,DECBIN3     ;if - 
	MOV   A,R2
	ORL   A,#80H             ;mark negative
	MOV   R2,A
	INC   DPTR
DECBIN3:MOV   R7,DPL             ;save begin of digits
	MOV   R6,DPH
DECBIN4:MOVX  A,@DPTR
        CJNE  A,#2EH,DECBIN5     ;if .
	MOV   A,R2               ;verify in only one .
	ANL   A,#7FH
	XRL   A,#7FH
	JNZ   DECBIN12           ;if second .
	MOV   A,R2
	MOV   C,ACC.7
	MOV   A,R3
	MOV   ACC.7,C
	MOV   R2,A
	INC   DPTR
	SJMP  DECBIN4            ;next
DECBIN5:CJNE  A,#30H,$+3         ;if no digit 
	JC    DECBIN6            ;end of string
	CJNE  A,#3AH,$+3
	JNC   DECBIN6
        INC   R3                 ;if digit inc counter
	INC   DPTR
        SJMP  DECBIN4
DECBIN6:MOV   DPL,R7
	MOV   DPH,R6
	MOV   A,R2
	ANL   A,#7FH	
	MOV   R4,A
	MOV   A,R3
	CLR   C
	SUBB  A,R4
	JNC   DECBIN7
	CLR   A
DECBIN7:
	CJNE  A,#08H,$+3
	JNC   DECBIN12
	SWAP  A
	ANL   A,#70H
	XCH   A,R2
	ANL   A,#80H
	ORL   A,R2
	MOV   R2,A	
	CLR   A
	MOV   R4,A
	MOV   R5,A
	MOV   R6,A
	MOV   R7,A
	MOV   A,R3
	JZ    DECBIN12
DECBIN8:MOVX  A,@DPTR
	INC   DPTR
	CJNE  A,#2EH,DECBIN9
	SJMP  DECBIN8
DECBIN9:ANL   A,#0FH
	MOV   R7,A
	DEC   R3
	MOV   A,R3
	JZ    DECBIN14
DECBIN10:
	MOV   A,R7
	MOV   B,#0AH
	MUL   AB
	MOV   R7,A
	MOV   R1,B
	MOV   A,R6
	MOV   B,#0AH
	MUL   AB
	ADD   A,R1
	MOV   R6,A
	MOV   A,B
	ADDC  A,#00H
	MOV   R1,A
	MOV   A,R5
	MOV   B,#0AH
	MUL   AB
	ADD   A,R1
	MOV   R5,A
	MOV   A,B
	ADDC  A,#00H
	MOV   R1,A
	MOV   A,R4
	MOV   B,#0AH
	MUL   AB
	ADD   A,R1
	MOV   R4,A
DECBIN11:
	MOVX  A,@DPTR
	INC   DPTR
	CJNE  A,#2EH,DECBIN13
	SJMP  DECBIN11
DECBIN12:                        ;if error
	CLR   A
	CPL   A
	SJMP  DECBIN16
DECBIN13:
	ANL   A,#0FH
	ADD   A,R7
	MOV   R7,A
	MOV   A,R6
	ADDC  A,#00H
	MOV   R6,A
	MOV   A,R5
	ADDC  A,#00H
	MOV   R5,A
	MOV   A,R4
	ADDC  A,#00H
	MOV   R4,A
	DJNZ  R3,DECBIN10
DECBIN14:
	MOV   A,R4
	ANL   A,#0F0H
	JNZ   DECBIN16
	MOV   A,MPAGE
	CJNE  A,#0F0H,DECBIN15
	MOV   A,R7               ;write to ram
	MOV   @R0,A
	INC   R0
	MOV   A,R6
	MOV   @R0,A
	INC   R0
	MOV   A,R5
	MOV   @R0,A
	INC   R0
	MOV   A,R4
	ANL   A,#0FH
	ORL   A,R2
	MOV   @R0,A
	CLR   A
	SJMP  DECBIN16
DECBIN15:
	MOV   A,R7               ;write to xram
	MOVX  @R0,A
	INC   R0
	MOV   A,R6
	MOVX  @R0,A
	INC   R0
	MOV   A,R5
	MOVX  @R0,A
	INC   R0
	MOV   A,R4
	ANL   A,#0FH
	ORL   A,R2
	MOVX  @R0,A
	CLR   A
DECBIN16:
	POP   MPAGE
	RET

;******************************************************;
; BINDEC in - @(R1:R0),  out: @DPTR
; used A,B,R0-R7
;******************************************************;
BINDEC: PUSH  MPAGE
	PUSH  DPL
	PUSH  DPH
	MOV   MPAGE,R1	
	MOV   A,R1
	CJNE  A,#0F0H,BINDEC1
	MOV   A,@R0              ;read from ram
	MOV   DPL,A
	INC   R0
	MOV   A,@R0
	MOV   DPH,A
	INC   R0
	MOV   A,@R0
	MOV   R1,A	
	INC   R0
	MOV   A,@R0
	MOV   R0,A
	SJMP  BINDEC2
BINDEC1:MOVX  A,@R0              ;read from xram
	MOV   DPL,A
	INC   R0
	MOVX  A,@R0
	MOV   DPH,A
	INC   R0
	MOVX  A,@R0
	MOV   R1,A	
	INC   R0
	MOVX  A,@R0
	MOV   R0,A
BINDEC2:ANL   A,#0FH             ;clear sign,point bits
	XCH   A,R0               ;bin: R0:R1:DPH:DPL
	MOV   C,ACC.7
	SWAP  A
	ANL   A,#07H
	MOV   ACC.7,C
	MOV   B,A                ;save sign bit in B
	CLR   A
	MOV   R3,A               ;dec R3:R4:R5:R6:R7
	MOV   R4,A
	MOV   R5,A	
	MOV   R6,A
	MOV   R7,A
	MOV   R2,#20H
BINDEC3:CLR   C                  ;rorate all left (*2)
	MOV   A,DPL
	RLC   A
	MOV   DPL,A		
	MOV   A,DPH
	RLC   A
	MOV   DPH,A
	MOV   A,R1
	RLC   A
	MOV   R1,A		
	MOV   A,R0
	RLC   A
	MOV   R0,A		
	MOV   A,R7
	ADDC  A,ACC
	DA    A
	MOV   R7,A		
	MOV   A,R6
	ADDC  A,ACC
	DA    A
	MOV   R6,A		
	MOV   A,R5
	ADDC  A,ACC
	DA    A
	MOV   R5,A		
	MOV   A,R4
	ADDC  A,ACC
	DA    A
	MOV   R4,A		
	MOV   A,R3
	ADDC  A,ACC
	DA    A
	MOV   R3,A		
	DJNZ  R2,BINDEC3
	POP   DPH
	POP   DPL
	MOV   A,B                ;if negative
	ANL   A,#0FH
	MOV   R2,A
	MOV   A,#0AH
	CLR   C
	SUBB  A,R2
	XCH   A,B
	JNB   ACC.7,BINDEC4
	MOV   A,#2DH             ;put -
	MOVX  @DPTR,A
	INC   DPTR
BINDEC4:CLR   A
	MOV   R1,A
BINDEC5:MOV   R2,A
	MOV   A,B                ;verify if decimal point needed
	CJNE  A,02H,BINDEC7      ;no
	MOV   A,R1               ;yes
	JNZ   BINDEC6            ;if no digits yet
	MOV   A,#30H             ;put zero
	MOVX  @DPTR,A
	INC   DPTR
	INC   R1
BINDEC6:MOV   A,#2EH             ;put .
	MOVX  @DPTR,A
	INC   DPTR
BINDEC7:MOV   A,R2
	RR    A
	MOV   R0,A
	ANL   A,#0FH
	ADD   A,#03H
	XCH   A,R0
	MOV   C,ACC.7
	MOV   A,@R0
	JC    BINDEC8
	SWAP  A
BINDEC8:ANL   A,#0FH
	ORL   A,#30H
	CJNE  A,#30H,BINDEC10
	MOV   A,R2
	XRL   A,#09H
	JZ    BINDEC9
	MOV   A,R1
	JZ    BINDEC11
BINDEC9:MOV   A,#30H
BINDEC10:
	MOVX  @DPTR,A
	INC   DPTR
	INC   R1
BINDEC11:
	MOV   A,R2
	INC   A
	CJNE  A,#0AH,BINDEC5
	POP   MPAGE
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
	.DB 6,2AH,0F0H,0F0H,0F0H,0F0H,0F0H ;rx addr    server side
	.DB 6,30H,0F0H,0F0H,0F0H,0F0H,0F0H ;tx addr
	.DB 6,2BH,0D2H,0F0H,0F0H,0F0H,0F0H ;RX_ADDR_P1
;
	.DB 2,31H,RFRXRWP        ;20H            ;RX_PW_P1 32 bytes in pipe 0
	.DB 2,32H,RFRXRWP        ;20H            ;RX_PW_P1 32 bytes in pipe 1
	.DB 2,25H,RFCH           ;channel 100
	.DB 2,3DH,00H            ;feature
	.DB 2,3CH,00H            ;DYNPD dynamic payload for all pipes disabled
	.DB 2,20H,0EH            ;Enable crc, crc 2 byte, PTX, power up
	.DB 0
NRPRFX: .TEXT nrf
	.DB NRFID
	.TEXT /
	.DB 0
NRCMTX: .TEXT nrfcmd
	.DB 0
NRECTX: .TEXT .
	.DB 0
NRRSTX: .TEXT nrfrsp
	.DB 0
NRCMGT: .TEXT get
	.DB 0
NRCMAD: .TEXT add
	.DB 0
NRCMDL: .TEXT del
	.DB 0
NRFWLD: .TEXT ldfw
	.DB 0
NROKTH  .TEXT :
NROKTX  .TEXT Ok
        .DB 0
NRERTH  .TEXT :
NRERTX  .TEXT Error
        .DB 0
NRTRTX  .TEXT true
        .DB 0
NRFSTX  .TEXT false
        .DB 0
NRONTX  .TEXT on
        .DB 0
NROFTX  .TEXT off
        .DB 0
NRBTTX  .TEXT _bytes
        .DB 0
NRNNTX  .TEXT NoName
        .DB 0
NRNRFTX .TEXT NoRFL
        .DB 0
NRUPTX  .TEXT Updating...
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

