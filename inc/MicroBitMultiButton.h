#ifndef MICROBIT_MULTI_BUTTON_H
#define MICROBIT_MULTI_BUTTON_H

#include "MicroBit.h"

#define MICROBIT_MULTI_BUTTON_STATE_1               0x01
#define MICROBIT_MULTI_BUTTON_STATE_2               0x02
#define MICROBIT_MULTI_BUTTON_HOLD_TRIGGERED_1      0x04
#define MICROBIT_MULTI_BUTTON_HOLD_TRIGGERED_2      0x08
#define MICROBIT_MULTI_BUTTON_SUPRESSED_1           0X10 
#define MICROBIT_MULTI_BUTTON_SUPRESSED_2           0x20

/**
  * Class definition for MicroBitMultiButton.
  *
  * Represents a virtual button, capable of reacting to simultaneous presses of multiple 
  * other buttons.
  */
class MicroBitMultiButton : public MicroBitComponent
{
    uint16_t    button1;        // ID of the first button we're monitoring
    uint16_t    button2;        // ID of the second button we're monitoring

    uint16_t    otherSubButton(uint16_t b);
    int         isSubButtonPressed(uint16_t button);
    int         isSubButtonHeld(uint16_t button);
    int         isSubButtonSupressed(uint16_t button);
    void        setButtonState(uint16_t button, int value);
    void        setHoldState(uint16_t button, int value);
    void        setSupressedState(uint16_t button, int value);

    public:

    /**
      * Constructor. 
      * Create a representation of a vurtual button, that generates events based upon the combination
      * of two given buttons.
      * @param id the ID of the new MultiButton object.
      * @param button1 the ID of the first button to integrate.
      * @param button2 the ID of the second button to integrate.
      * @param name the physical pin on the processor that this butotn is connected to.
      *
      * Example:
      * @code 
      * multiButton(MICROBIT_ID_BUTTON_AB, MICROBIT_ID_BUTTON_A, MICROBIT_ID_BUTTON_B); 
      * @endcode
      *
      * Possible Events:
      * @code 
      * MICROBIT_BUTTON_EVT_DOWN
      * MICROBIT_BUTTON_EVT_UP
      * MICROBIT_BUTTON_EVT_CLICK
      * MICROBIT_BUTTON_EVT_LONG_CLICK
      * MICROBIT_BUTTON_EVT_DOUBLE_CLICK
      * MICROBIT_BUTTON_EVT_HOLD
      * @endcode
      */  
    MicroBitMultiButton(uint16_t id, uint16_t button1, uint16_t button2);    

    /**
      * Tests if this MultiButton is currently pressed.
      * @return 1 if both physical buttons are pressed simultaneously.
      *
      * Example:
      * @code 
      * if(uBit.buttonAB.isPressed())
      *     print("Pressed!");
      * @endcode
      */
    int isPressed();
    
    void onEvent(MicroBitEvent evt);
};

#endif
