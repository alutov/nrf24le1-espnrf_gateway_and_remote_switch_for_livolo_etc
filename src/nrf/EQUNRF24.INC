B20.0  .EQU 00H
B20.1  .EQU 01H
B20.2  .EQU 02H
B20.3  .EQU 03H
B20.4  .EQU 04H
B20.5  .EQU 05H
B20.6  .EQU 06H
B20.7  .EQU 07H
B21.0  .EQU 08H
B21.1  .EQU 09H
B21.2  .EQU 0AH
B21.3  .EQU 0BH
B21.4  .EQU 0CH
B21.5  .EQU 0DH
B21.6  .EQU 0EH
B21.7  .EQU 0FH

B22.0  .EQU 10H
B22.1  .EQU 11H
B22.2  .EQU 12H
B22.3  .EQU 13H
B22.4  .EQU 14H
B22.5  .EQU 15H
B22.6  .EQU 16H
B22.7  .EQU 17H
B23.0  .EQU 18H
B23.1  .EQU 19H
B23.2  .EQU 1AH
B23.3  .EQU 1BH
B23.4  .EQU 1CH
B23.5  .EQU 1DH
B23.6  .EQU 1EH
B23.7  .EQU 1FH

B24.0  .EQU 20H
B24.1  .EQU 21H
B24.2  .EQU 22H
B24.3  .EQU 23H
B24.4  .EQU 24H
B24.5  .EQU 25H
B24.6  .EQU 26H
B24.7  .EQU 27H
B25.0  .EQU 28H
B25.1  .EQU 29H
B25.2  .EQU 2AH
B25.3  .EQU 2BH
B25.4  .EQU 2CH
B25.5  .EQU 2DH
B25.6  .EQU 2EH
B25.7  .EQU 2FH

B26.0  .EQU 30H
B26.1  .EQU 31H
B26.2  .EQU 32H
B26.3  .EQU 33H
B26.4  .EQU 34H
B26.5  .EQU 35H
B26.6  .EQU 36H
B26.7  .EQU 37H
B27.0  .EQU 38H
B27.1  .EQU 39H
B27.2  .EQU 3AH
B27.3  .EQU 3BH
B27.4  .EQU 3CH
B27.5  .EQU 3DH
B27.6  .EQU 3EH
B27.7  .EQU 3FH

B28.0  .EQU 40H
B28.1  .EQU 41H
B28.2  .EQU 42H
B28.3  .EQU 43H
B28.4  .EQU 44H
B28.5  .EQU 45H
B28.6  .EQU 46H
B28.7  .EQU 47H
B29.0  .EQU 48H
B29.1  .EQU 49H
B29.2  .EQU 4AH
B29.3  .EQU 4BH
B29.4  .EQU 4CH
B29.5  .EQU 4DH
B29.6  .EQU 4EH
B29.7  .EQU 4FH

P0        .EQU  080H    ;Port 0
P0.0      .EQU  080H    ;Port 0 bit 0
P0.1      .EQU  081H    ;Port 0 bit 1
P0.2      .EQU  082H    ;Port 0 bit 2
P0.3      .EQU  083H    ;Port 0 bit 3
P0.4      .EQU  084H    ;Port 0 bit 4
P0.5      .EQU  085H    ;Port 0 bit 5
P0.6      .EQU  086H    ;Port 0 bit 6
P0.7      .EQU  087H    ;Port 0 bit 7

SP        .EQU  081H    ;Stack pointer

DPL       .EQU  082H    ;
DPH       .EQU  083H    ;
DPL1      .EQU  084H    ;
DPH1      .EQU  085H    ;
PCON      .EQU  087H    ;

TCON      .EQU  088H    ;*
TCON.0    .EQU  088H    ;bit 0
TCON.1    .EQU  089H    ;bit 1
TCON.2    .EQU  08AH    ;bit 2
TCON.3    .EQU  08BH    ;bit 3
TCON.4    .EQU  08CH    ;bit 4
TCON.5    .EQU  08DH    ;bit 5
TCON.6    .EQU  08EH    ;bit 6
TCON.7    .EQU  08FH    ;bit 7

TMOD      .EQU  089H    ;
TL0       .EQU  08AH    ;
TL1       .EQU  08BH    ;
TH0       .EQU  08CH    ;
TH1       .EQU  08DH    ;
P3CON     .EQU  08FH    ;

P1        .EQU  090H    ;Port 1
P1.0      .EQU  090H    ;Port 1 bit 0
P1.1      .EQU  091H    ;Port 1 bit 1
P1.2      .EQU  092H    ;Port 1 bit 2
P1.3      .EQU  093H    ;Port 1 bit 3
P1.4      .EQU  094H    ;Port 1 bit 4
P1.5      .EQU  095H    ;Port 1 bit 5
P1.6      .EQU  096H    ;Port 1 bit 6
P1.7      .EQU  097H    ;Port 1 bit 7

DPS       .EQU  092H    ;
P0DIR     .EQU  093H    ;
P1DIR     .EQU  094H    ;
P2DIR     .EQU  095H    ;
P3DIR     .EQU  096H    ;
P2CON     .EQU  097H    ;

S0CON     .EQU  098H    ;*
S0CON.0   .EQU  098H    ;bit 0
S0CON.1   .EQU  099H    ;bit 1
S0CON.2   .EQU  09AH    ;bit 2
S0CON.3   .EQU  09BH    ;bit 3
S0CON.4   .EQU  09CH    ;bit 4
S0CON.5   .EQU  09DH    ;bit 5
S0CON.6   .EQU  09EH    ;bit 6
S0CON.7   .EQU  09FH    ;bit 7

S0BUF     .EQU  099H
P0CON     .EQU  09EH    ;
P1CON     .EQU  09FH    ;

P2        .EQU  0A0H    ;Port 2
P2.0      .EQU  0A0H    ;Port 2 bit 0
P2.1      .EQU  0A1H    ;Port 2 bit 1
P2.2      .EQU  0A2H    ;Port 2 bit 2
P2.3      .EQU  0A3H    ;Port 2 bit 3
P2.4      .EQU  0A4H    ;Port 2 bit 4
P2.5      .EQU  0A5H    ;Port 2 bit 5
P2.6      .EQU  0A6H    ;Port 2 bit 6
P2.7      .EQU  0A7H    ;Port 2 bit 7

PWMDC0    .EQU  0A1H    ;
PWMDC1    .EQU  0A2H    ;
CLKCTRL   .EQU  0A3H    ;
PWRDWN    .EQU  0A4H    ;
WUCON     .EQU  0A5H    ;
INTEXP    .EQU  0A6H    ;
MEMCON    .EQU  0A7H    ;

IEN0      .EQU  0A8H    ;*
IEN0.0    .EQU  0A8H    ;bit 0
IEN0.1    .EQU  0A9H    ;bit 1
IEN0.2    .EQU  0AAH    ;bit 2
IEN0.3    .EQU  0ABH    ;bit 3
IEN0.4    .EQU  0ACH    ;bit 4
IEN0.5    .EQU  0ADH    ;bit 5
IEN0.6    .EQU  0AEH    ;bit 6
IEN0.7    .EQU  0AFH    ;bit 7

IP0       .EQU  0A9H    ;
S0RELL    .EQU  0AAH    ;
RTC2CPT01 .EQU  0ABH    ;
RTC2CPT10 .EQU  0ACH    ;
CLKLFCTRL .EQU  0ADH    ;
OPMCON    .EQU  0AEH    ;
WDSV      .EQU  0AFH    ;

P3        .EQU  0B0H    ;Port 3
P3.0      .EQU  0B0H    ;Port 3 bit 0
P3.1      .EQU  0B1H    ;Port 3 bit 1
P3.2      .EQU  0B2H    ;Port 3 bit 2
P3.3      .EQU  0B3H    ;Port 3 bit 3
P3.4      .EQU  0B4H    ;Port 3 bit 4
P3.5      .EQU  0B5H    ;Port 3 bit 5
P3.6      .EQU  0B6H    ;Port 3 bit 6
P3.7      .EQU  0B7H    ;Port 3 bit 7

RSTREAS   .EQU  0B1H    ;
PWMCON    .EQU  0B2H    ;
RTC2CON   .EQU  0B3H    ;
RTC2CMP0  .EQU  0B4H    ;
RTC2CMP1  .EQU  0B5H    ;
RTC2CPT00 .EQU  0B6H    ;

IEN1      .EQU  0B8H    ;*
IEN1.0    .EQU  0B8H    ;bit 0
IEN1.1    .EQU  0B9H    ;bit 1
IEN1.2    .EQU  0BAH    ;bit 2
IEN1.3    .EQU  0BBH    ;bit 3
IEN1.4    .EQU  0BCH    ;bit 4
IEN1.5    .EQU  0BDH    ;bit 5
IEN1.6    .EQU  0BEH    ;bit 6
IEN1.7    .EQU  0BFH    ;bit 7

IP1       .EQU  0B9H    ;
S0RELH    .EQU  0BAH    ;
SPISCON0  .EQU  0BCH    ;
SPISSTAT  .EQU  0BEH    ;
SPISDAT   .EQU  0BFH    ;

IRCON     .EQU  0C0H    ;*
IRCON.0   .EQU  0C0H    ;bit 0
IRCON.1   .EQU  0C1H    ;bit 1
IRCON.2   .EQU  0C2H    ;bit 2
IRCON.3   .EQU  0C3H    ;bit 3
IRCON.4   .EQU  0C4H    ;bit 4
IRCON.5   .EQU  0C5H    ;bit 5
IRCON.6   .EQU  0C6H    ;bit 6
IRCON.7   .EQU  0C7H    ;bit 7

CCEN      .EQU  0C1H    ;
CCL1      .EQU  0C2H    ;
CCH1      .EQU  0C3H    ;
CCL2      .EQU  0C4H    ;
CCH2      .EQU  0C5H    ;
CCL3      .EQU  0C6H    ;
CCH3      .EQU  0C7H    ;

T2CON     .EQU  0C8H    ;
T2CON.0   .EQU  0C8H    ;bit 0
T2CON.1   .EQU  0C9H    ;bit 1
T2CON.2   .EQU  0CAH    ;bit 2
T2CON.3   .EQU  0CBH    ;bit 3
T2CON.4   .EQU  0CCH    ;bit 4
T2CON.5   .EQU  0CDH    ;bit 5
T2CON.6   .EQU  0CEH    ;bit 6
T2CON.7   .EQU  0CFH    ;bit 7

MPAGE     .EQU  0C9H    ;
CRCL      .EQU  0CAH    ;
CRCH      .EQU  0CBH    ;
TL2       .EQU  0CCH    ;
TH2       .EQU  0CDH    ;
WUOPC1    .EQU  0CEH    ;
WUOPC0    .EQU  0CFH    ;

PSW       .EQU  0D0H
PSW.0     .EQU  0D0H    ; bit 0
PSW.1     .EQU  0D1H    ; bit 1
PSW.2     .EQU  0D2H    ; bit 2
PSW.3     .EQU  0D3H    ; bit 3
PSW.4     .EQU  0D4H    ; bit 4
PSW.5     .EQU  0D5H    ; bit 5
PSW.6     .EQU  0D6H    ; bit 6
PSW.7     .EQU  0D7H    ; bit 7

ADCCON3   .EQU  0D1H    ;
ADCCON2   .EQU  0D2H    ;
ADCCON1   .EQU  0D3H    ;
ADCDATH   .EQU  0D4H    ;
ADCDATL   .EQU  0D5H    ;
RNGCTL    .EQU  0D6H    ;
RNGDAT    .EQU  0D7H    ;

ADCON     .EQU  0D8H    ;
ADCON.0   .EQU  0D8H    ;bit 0
ADCON.1   .EQU  0D9H    ;bit 1
ADCON.2   .EQU  0DAH    ;bit 2
ADCON.3   .EQU  0DBH    ;bit 3
ADCON.4   .EQU  0DCH    ;bit 4
ADCON.5   .EQU  0DDH    ;bit 5
ADCON.6   .EQU  0DEH    ;bit 6
ADCON.7   .EQU  0DFH    ;bit 7

W2SADR    .EQU  0D9H    ;
W2DAT     .EQU  0DAH    ;
COMPCON   .EQU  0DBH    ;
POFCON    .EQU  0DCH    ;
CCPDATIA  .EQU  0DDH    ;
CCPDATIB  .EQU  0DEH    ;
CCPDATO   .EQU  0DFH    ;

ACC       .EQU  0E0H    ;Accumulator
ACC.0     .EQU  0E0H    ;Acc bit 0
ACC.1     .EQU  0E1H    ;Acc bit 1
ACC.2     .EQU  0E2H    ;Acc bit 2
ACC.3     .EQU  0E3H    ;Acc bit 3
ACC.4     .EQU  0E4H    ;Acc bit 4
ACC.5     .EQU  0E5H    ;Acc bit 5
ACC.6     .EQU  0E6H    ;Acc bit 6
ACC.7     .EQU  0E7H    ;Acc bit 7

W2CON1    .EQU  0E1H    ;
W2CON0    .EQU  0E2H    ;
SPIRCON0  .EQU  0E4H    ;
SPIRCON1  .EQU  0E5H    ;
SPIRSTAT  .EQU  0E6H    ;
SPIRDAT   .EQU  0E7H    ;

RFCON     .EQU  0E8H    ;
RFCON.0   .EQU  0E8H    ;bit 0
RFCON.1   .EQU  0E9H    ;bit 1
RFCON.2   .EQU  0EAH    ;bit 2
RFCON.3   .EQU  0EBH    ;bit 3
RFCON.4   .EQU  0ECH    ;bit 4
RFCON.5   .EQU  0EDH    ;bit 5
RFCON.6   .EQU  0EEH    ;bit 6
RFCON.7   .EQU  0EFH    ;bit 7

B         .EQU  0F0H    ;Secondary Accumulator
B.0       .EQU  0F0H    ;Acc bit 0
B.1       .EQU  0F1H    ;Acc bit 1
B.2       .EQU  0F2H    ;Acc bit 2
B.3       .EQU  0F3H    ;Acc bit 3
B.4       .EQU  0F4H    ;Acc bit 4
B.5       .EQU  0F5H    ;Acc bit 5
B.6       .EQU  0F6H    ;Acc bit 6
B.7       .EQU  0F7H    ;Acc bit 7

FSR       .EQU  0F8H    ;
FSR.0     .EQU  0F8H    ;bit 0
FSR.1     .EQU  0F9H    ;bit 1
FSR.2     .EQU  0FAH    ;bit 2
FSR.3     .EQU  0FBH    ;bit 3
FSR.4     .EQU  0FCH    ;bit 4
FSR.5     .EQU  0FDH    ;bit 5
FSR.6     .EQU  0FEH    ;bit 6
FSR.7     .EQU  0FFH    ;bit 7

FPCR      .EQU  0F9H    ;
FCR       .EQU  0FAH    ;

SPIMCON0  .EQU  0FCH    ;
SPIMCON1  .EQU  0FDH    ;
SPIMSTAT  .EQU  0FEH    ;
SPIMDAT   .EQU  0FFH    ;
