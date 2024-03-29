#include "MicroBit.h"

/**
  * Constructor.
  * Create a pin representation with the given ID.
  * @param id the ID of the new MicroBitButton object.
  * @param name the physical pin on the processor that this butotn is connected to.
  * @param mode the configuration of internal pullups/pulldowns, as define in the mbed PinMode class. PullNone by default.
  *
  * Example:
  * @code
  * buttonA(MICROBIT_ID_BUTTON_A,MICROBIT_PIN_BUTTON_A); //a number between 0 and 200 inclusive
  * @endcode
  *
  * Possible Events:
  * @code
  * MICROBIT_BUTTON_EVT_DOWN
  * MICROBIT_BUTTON_EVT_UP
  * MICROBIT_BUTTON_EVT_CLICK
  * MICROBIT_BUTTON_EVT_LONG_CLICK
  * MICROBIT_BUTTON_EVT_HOLD
  * @endcode
  */
MicroBitButton::MicroBitButton(uint16_t id, PinName name, MicroBitButtonEventConfiguration eventConfiguration, PinMode mode) : pin(name, mode)
{
    this->id = id;
    this->name = name;
    this->eventConfiguration = eventConfiguration;
    this->downStartTime = 0;
    this->sigma = 0;
    uBit.addSystemComponent(this);
}

/**
  * periodic callback from MicroBit clock.
  * Check for state change for this button, and fires a hold event if button is pressed.
  */
void MicroBitButton::systemTick()
{
    //
    // If the pin is pulled low (touched), increment our culumative counter.
    // otherwise, decrement it. We're essentially building a lazy follower here.
    // This makes the output debounced for buttons, and desensitizes touch sensors
    // (particularly in environments where there is mains noise!)
    //
    if(!pin)
    {
        if (sigma < MICROBIT_BUTTON_SIGMA_MAX)
            sigma++;
    }
    else
    {
        if (sigma > MICROBIT_BUTTON_SIGMA_MIN)
            sigma--;
    }

    // Check to see if we have off->on state change.
    if(sigma > MICROBIT_BUTTON_SIGMA_THRESH_HI && !(status & MICROBIT_BUTTON_STATE))
    {
        // Record we have a state change, and raise an event.
        status |= MICROBIT_BUTTON_STATE;
        MicroBitEvent evt(id,MICROBIT_BUTTON_EVT_DOWN);

        //Record the time the button was pressed.
        downStartTime=ticks;
    }

    // Check to see if we have on->off state change.
    if(sigma < MICROBIT_BUTTON_SIGMA_THRESH_LO && (status & MICROBIT_BUTTON_STATE))
    {
        status = 0;
        MicroBitEvent evt(id,MICROBIT_BUTTON_EVT_UP);

       if (eventConfiguration == MICROBIT_BUTTON_ALL_EVENTS)
       {
           //determine if this is a long click or a normal click and send event
           if((ticks - downStartTime) >= MICROBIT_BUTTON_LONG_CLICK_TIME)
               MicroBitEvent evt(id,MICROBIT_BUTTON_EVT_LONG_CLICK);
           else
               MicroBitEvent evt(id,MICROBIT_BUTTON_EVT_CLICK);
       }
    }

    //if button is pressed and the hold triggered event state is not triggered AND we are greater than the button debounce value
    if((status & MICROBIT_BUTTON_STATE) && !(status & MICROBIT_BUTTON_STATE_HOLD_TRIGGERED) && (ticks - downStartTime) >= MICROBIT_BUTTON_HOLD_TIME)
    {
        //set the hold triggered event flag
        status |= MICROBIT_BUTTON_STATE_HOLD_TRIGGERED;

        //fire hold event
        MicroBitEvent evt(id,MICROBIT_BUTTON_EVT_HOLD);
    }
}

/**
  * Tests if this Button is currently pressed.
  * @return 1 if this button is pressed, 0 otherwise.
  */
int MicroBitButton::isPressed()
{
    return status & MICROBIT_BUTTON_STATE ? 1 : 0;
}

/**
  * Destructor for MicroBitButton, so that we deregister ourselves as a systemComponent
  */
MicroBitButton::~MicroBitButton()
{
    uBit.removeSystemComponent(this);
}
