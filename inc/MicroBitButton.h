/**
  * Class definition for MicroBit Button.
  *
  * Represents a single button on the device.
  */
  
#ifndef MICROBIT_BUTTON_H
#define MICROBIT_BUTTON_H

#include "mbed.h"

#define MICROBIT_PIN_LEFT_BUTTON          P0_23

#define MICROBIT_BUTTON_EVT_DOWN          1
#define MICROBIT_BUTTON_EVT_UP            2



class MicroBitButton
{
    /**
      * Unique, enumerated ID for this component. 
      * Used to track asynchronous events in the event bus.
      */
      
    int id;             // Event Bus ID
    PinName name;       // mBed pin name of this pin.
    DigitalIn pin;      // The mBed object looking after this pin at any point in time (may change!).
    InterruptIn irq;    // Handler to detect change events.
       
    void rising();      // Interrupt on change handler
    void falling();     // Interrupt on change handler
    
    public:
    /**
      * Constructor. 
      * Create a pin representation with the given ID.
      * @param id the ID of the new LED object.
      * @param name the physical pin on the processor that this butotn is connected to.
      */
    MicroBitButton(int id, PinName name);
    
    /**
      * Tests if this Button is currently pressed.
      * @return 1 if this button is pressed, 0 otherwise.
      */
    int isPressed();
    
};

#endif
