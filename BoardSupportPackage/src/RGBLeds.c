#include "msp.h"
#include "RGBLeds.h"

void init_RGBLEDS(){
    uint16_t UNIT_OFF = 0x0000;

    UCB2CTLW0 |= UCSWRST;

    UCB2CTLW0 |= UCTR | UCSYNC | UCMST | UCMODE_3 | UCSSEL_3;

    UCB2BRW = 30;

    P3SEL0 |= 0xC0; // bit 7 & 6
    P3SEL1 &= 0x3F;

    UCB2CTLW0 &= ~UCSWRST;

    LP3943_LedModeSet(RED, UNIT_OFF);
    LP3943_LedModeSet(GREEN, UNIT_OFF);
    LP3943_LedModeSet(BLUE, UNIT_OFF);
}

void LP3943_LedModeSet(uint32_t unit, uint16_t LED_DATA){
//0x55
    uint8_t Data6 = LED_DATA & 0x000F;
    Data6 = ((Data6 & 0x08)<<3) | ((Data6 & 0x04)<<2) | ((Data6 & 0x02)<<1) | ((Data6 & 0x01));
    uint8_t Data7 = (LED_DATA & 0x00F0) >> 4;
    Data7 = ((Data7 & 0x08)<<3) | ((Data7 & 0x04)<<2) | ((Data7 & 0x02)<<1) | ((Data7 & 0x01));
    uint8_t Data8 = (LED_DATA & 0x0F00) >> 8;
    Data8 = ((Data8 & 0x08)<<3) | ((Data8 & 0x04)<<2) | ((Data8 & 0x02)<<1) | ((Data8 & 0x01));
    uint8_t Data9 = (LED_DATA & 0xF000) >> 12;
    Data9 = ((Data9 & 0x08)<<3) | ((Data9 & 0x04)<<2) | ((Data9 & 0x02)<<1) | ((Data9 & 0x01));

    if (unit == RED){
        UCB2I2CSA = 0x62;
    }
    else if (unit == GREEN){
        UCB2I2CSA = 0x61;
    }
    else {
        UCB2I2CSA = 0x60;
    }


    int k = 0x06;
    int i = 0;

    for (i = 0; i < 4; i++){
        UCB2CTLW0 |= UCTXSTT; //transmit start condition
        while ((UCB2CTLW0 & UCTXSTT) != 0);//check actual bit instead
        while ((UCB2IFG & UCTXIFG0) == 0);
        UCB2TXBUF  = k + i; //which LS
        while ((UCB2IFG & UCTXIFG0) == 0);
        if (i == 0){
            UCB2TXBUF = Data6; //LEDs to light up
        }
        else if ( i == 1){
            UCB2TXBUF = Data7; //LEDs to light up
        }
        else if (i == 2){
            UCB2TXBUF = Data8; //LEDs to light up
        }
        else {
            UCB2TXBUF = Data9; //LEDs to light up
        }

        while ((UCB2IFG & UCTXIFG0) == 0);
        UCB2CTLW0 |= UCTXSTP; //transmit stop condition
        while((UCB2CTLW0 & UCTXSTP) != 0);
    }
}
