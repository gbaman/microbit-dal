/**
  * Class definition for MicroBit IO.
  *
  * Represents a single IO pin on the edge connector.
  */

#include "MicroBit.h"

/**
  * Constructor. 
  * Create a representation of all given I/O pins on the edge connector
  */
MicroBitIO::MicroBitIO(int ID_P0, int ID_P1, int ID_P2,
                       int ID_P3, int ID_P4, int ID_P5,
                       int ID_P6, int ID_P7, int ID_P8,
                       int ID_P9, int ID_P10,int ID_P11,
                       int ID_P12,int ID_P13,int ID_P14,
                       int ID_P15,int ID_P16,int ID_P19,
                       int ID_P20) :
    P0 (ID_P0, MICROBIT_PIN_P0, PIN_CAPABILITY_ALL),            //P0 is the left most pad (ANALOG/DIGITAL/TOUCH) 
    P1 (ID_P1, MICROBIT_PIN_P1, PIN_CAPABILITY_ALL),            //P1 is the middle pad (ANALOG/DIGITAL/TOUCH)
    P2 (ID_P2, MICROBIT_PIN_P2, PIN_CAPABILITY_ALL),            //P2 is the right most pad (ANALOG/DIGITAL/TOUCH)
    P3 (ID_P3, MICROBIT_PIN_P3, PIN_CAPABILITY_AD),             //COL1 (ANALOG/DIGITAL) 
    P4 (ID_P4, MICROBIT_PIN_P4, PIN_CAPABILITY_DIGITAL),        //BTN_A 
    P5 (ID_P5, MICROBIT_PIN_P5, PIN_CAPABILITY_AD),             //COL2 (ANALOG/DIGITAL) 
    P6 (ID_P6, MICROBIT_PIN_P6, PIN_CAPABILITY_DIGITAL),        //ROW2
    P7 (ID_P7, MICROBIT_PIN_P7, PIN_CAPABILITY_DIGITAL),        //ROW1 
    P8 (ID_P8, MICROBIT_PIN_P8, PIN_CAPABILITY_DIGITAL),        //PIN 18
    P9 (ID_P9, MICROBIT_PIN_P9, PIN_CAPABILITY_DIGITAL),        //ROW3
    P10(ID_P10,MICROBIT_PIN_P10,PIN_CAPABILITY_AD),             //COL3 (ANALOG/DIGITAL) 
    P11(ID_P11,MICROBIT_PIN_P11,PIN_CAPABILITY_DIGITAL),        //BTN_B
    P12(ID_P12,MICROBIT_PIN_P12,PIN_CAPABILITY_DIGITAL),        //PIN 20
    P13(ID_P13,MICROBIT_PIN_P13,PIN_CAPABILITY_DIGITAL),        //SCK
    P14(ID_P14,MICROBIT_PIN_P14,PIN_CAPABILITY_DIGITAL),        //MISO
    P15(ID_P15,MICROBIT_PIN_P15,PIN_CAPABILITY_DIGITAL),        //MOSI
    P16(ID_P16,MICROBIT_PIN_P16,PIN_CAPABILITY_DIGITAL),        //PIN 16
    P19(ID_P19,MICROBIT_PIN_P19,PIN_CAPABILITY_DIGITAL),        //SCL
    P20(ID_P20,MICROBIT_PIN_P20,PIN_CAPABILITY_DIGITAL)         //SDA                 
{   
}
