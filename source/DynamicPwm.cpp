#include "MicroBit.h"

DynamicPwm* DynamicPwm::pwms[NO_PWMS] = { NULL };

uint8_t DynamicPwm::lastUsed = NO_PWMS+1; //set it to out of range i.e. 4 so we know it hasn't been used yet.

uint16_t DynamicPwm::sharedPeriod = 0; //set the shared period to an unknown state

/**
  * Reassigns an already operational PWM channel to the given pin
  * #HACK #BODGE # YUCK #MBED_SHOULD_DO_THIS
  *
  * @param pin The pin to start running PWM on
  * @param oldPin The pin to stop running PWM on
  * @param channel_number The GPIOTE channel being used to drive this PWM channel
  */
void gpiote_reinit(PinName pin, PinName oldPin, uint8_t channel_number)
{
    // Connect GPIO input buffers and configure PWM_OUTPUT_PIN_NUMBER as an output.
    NRF_GPIO->PIN_CNF[pin] = (GPIO_PIN_CNF_SENSE_Disabled << GPIO_PIN_CNF_SENSE_Pos)
                            | (GPIO_PIN_CNF_DRIVE_S0S1 << GPIO_PIN_CNF_DRIVE_Pos)
                            | (GPIO_PIN_CNF_PULL_Disabled << GPIO_PIN_CNF_PULL_Pos)
                            | (GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos)
                            | (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos);

    NRF_GPIO->OUTCLR = (1 << oldPin);
    NRF_GPIO->OUTCLR = (1 << pin);

    /* Finally configure the channel as the caller expects. If OUTINIT works, the channel is configured properly.
       If it does not, the channel output inheritance sets the proper level. */

    NRF_GPIOTE->CONFIG[channel_number] = (GPIOTE_CONFIG_MODE_Task << GPIOTE_CONFIG_MODE_Pos) |
                                         ((uint32_t)pin << GPIOTE_CONFIG_PSEL_Pos) |
                                         ((uint32_t)GPIOTE_CONFIG_POLARITY_Toggle << GPIOTE_CONFIG_POLARITY_Pos) |
                                         ((uint32_t)GPIOTE_CONFIG_OUTINIT_Low << GPIOTE_CONFIG_OUTINIT_Pos); // ((uint32_t)GPIOTE_CONFIG_OUTINIT_High <<
                                                                                                             // GPIOTE_CONFIG_OUTINIT_Pos);//

    /* Three NOPs are required to make sure configuration is written before setting tasks or getting events */
    __NOP();
    __NOP();
    __NOP();

    NRF_TIMER2->CC[channel_number] = 0;
}

/**
  * An internal constructor used when allocating a new DynamicPwm representation
  * @param pin the name of the pin for the pwm to target
  * @param persistance the level of persistence for this pin PWM_PERSISTENCE_PERSISTENT (can not be replaced until freed, should only be used for system services really.)
  * or PWM_PERSISTENCE_TRANSIENT (can be replaced at any point if a channel is required.)
  * @param period the frequency of the pwm channel in us.
  */
DynamicPwm::DynamicPwm(PinName pin, PwmPersistence persistence) : PwmOut(pin)
{
    this->flags = persistence;
}

/**
  * Redirects the pwm channel to point at a different pin.
  * @param pin the new pin to direct PWM at.
  *
  * Example:
  * @code
  * DynamicPwm* pwm = DynamicPwm::allocate(PinName n);
  * pwm->redirect(PinName n2); // pwm is now produced on n2
  * @endcode
  */
void DynamicPwm::redirect(PinName pin)
{
    gpiote_reinit(pin, _pwm.pin, (uint8_t)_pwm.pwm);
    this->_pwm.pin = pin;
}

/**
  * Retrieves a pointer to the first available free pwm channel - or the first one that can be reallocated.
  * @param pin the name of the pin for the pwm to target
  * @param persistance the level of persistence for this pin PWM_PERSISTENCE_PERSISTENT (can not be replaced until freed, should only be used for system services really.)
  * or PWM_PERSISTENCE_TRANSIENT (can be replaced at any point if a channel is required.)
  *
  * Example:
  * @code
  * DynamicPwm* pwm = DynamicPwm::allocate(PinName n);
  * @endcode
  */
DynamicPwm* DynamicPwm::allocate(PinName pin, PwmPersistence persistence)
{
    //try to find a blank spot first
    for(int i = 0; i < NO_PWMS; i++)
    {
        if(pwms[i] == NULL)
        {
            lastUsed = i;
            pwms[i] = new DynamicPwm(pin, persistence);
            return pwms[i];
        }
    }

    //no blank spot.. try to find a transient PWM
    int channelIterator = (lastUsed + 1 > NO_PWMS - 1) ? 0 : lastUsed + 1;

    while(channelIterator != lastUsed)
    {
        if(pwms[channelIterator]->flags & PWM_PERSISTENCE_TRANSIENT)
        {
            lastUsed = channelIterator;
            pwms[channelIterator]->flags = persistence;
            pwms[channelIterator]->redirect(pin);
            return pwms[channelIterator];
        }

        channelIterator = (channelIterator + 1 > NO_PWMS - 1) ? 0 : channelIterator + 1;
    }

    //if we haven't found a free one, we must try to allocate the last used...
    if(pwms[lastUsed]->flags & PWM_PERSISTENCE_TRANSIENT)
    {
        pwms[lastUsed]->flags = persistence;
        pwms[lastUsed]->redirect(pin);
        return pwms[lastUsed];
    }

    //well if we have no transient channels - we can't give any away! :( return null
    return (DynamicPwm*)NULL;
}

/**
  * Frees this DynamicPwm instance if the pointer is valid.
  *
  * Example:
  * @code
  * DynamicPwm* pwm = DynamicPwm::allocate();
  * pwm->release();
  * @endcode
  */
void DynamicPwm::release()
{
    //free the pwm instance.
    NRF_GPIOTE->CONFIG[(uint8_t) _pwm.pwm] = 0;
    pwmout_free(&_pwm);
    this->flags = PWM_PERSISTENCE_TRANSIENT;

    //set the pointer to this object to null...
    for(int i =0; i < NO_PWMS; i++)
        if(pwms[i] == this)
        {
            delete pwms[i];
            pwms[i] = NULL;
        }
}

/**
  * A lightweight wrapper around the super class' write in order to capture the value
  *
  * @param value the duty cycle percentage in floating point format.
  *
  * @return MICROBIT_OK on success, MICROBIT_INVALID_PARAMETER if value is out of range
  *
  * Example:
  * @code
  * DynamicPwm* pwm = DynamicPwm::allocate();
  * pwm->write(0.5);
  * @endcode
  */
int DynamicPwm::write(float value){

    if(value < 0)
        return MICROBIT_INVALID_PARAMETER;

    PwmOut::write(value);
    lastValue = value;

    return MICROBIT_OK;
}

/**
  * Retreives the pin name associated with this DynamicPwm instance.
  *
  * Example:
  * @code
  * DynamicPwm* pwm = DynamicPwm::allocate(PinName n);
  * pwm->getPinName(); // equal to n
  * @endcode
  */
PinName DynamicPwm::getPinName()
{
    return _pwm.pin;
}

/**
  * Retreives the last value that has been written to this DynamicPwm instance.
  * The value is in the range 0-1024
  *
  * Example:
  * @code
  * DynamicPwm* pwm = DynamicPwm::allocate(PinName n);
  * pwm->getValue();
  * @endcode
  */
int DynamicPwm::getValue()
{
    return (float)lastValue * float(MICROBIT_PIN_MAX_OUTPUT);
}

/**
  * Retreives the current period in use by the entire PWM module
  *
  * Example:
  * @code
  * DynamicPwm* pwm = DynamicPwm::allocate(PinName n);
  * pwm->getPeriod();
  * @endcode
  */
int DynamicPwm::getPeriodUs()
{
    return sharedPeriod;
}

/**
  * Retreives the current period in use by the entire PWM module
  *
  * Example:
  * @code
  * DynamicPwm* pwm = DynamicPwm::allocate(PinName n);
  * pwm->getPeriod();
  * @endcode
  */
int DynamicPwm::getPeriod()
{
    return getPeriodUs() / 1000;
}

/**
  * Sets the period used by the WHOLE PWM module. Any changes to the period will AFFECT ALL CHANNELS.
  *
  * @param period the desired period in microseconds.
  *
  * @return MICROBIT_OK on success, MICROBIT_INVALID_PARAMETER if value is out of range
  *
  * Example:
  * @code
  * DynamicPwm* pwm = DynamicPwm::allocate(PinName n);
  * pwm->setPeriodUs(1000); // period now is 1ms
  * @endcode
  */
int DynamicPwm::setPeriodUs(int period)
{
    if(period < 0)
        return MICROBIT_INVALID_PARAMETER;

    //#HACK this forces mbed to update the pulse width calculation.
    period_us(period);
    write(lastValue);
    sharedPeriod = period;

    return MICROBIT_OK;
}

/**
  * Sets the period used by the WHOLE PWM module. Any changes to the period will AFFECT ALL CHANNELS.
  *
  * @param period the desired period in microseconds.
  *
  * @return MICROBIT_OK on success, MICROBIT_INVALID_PARAMETER if value is out of range
  *
  * Example:
  * @code
  * DynamicPwm* pwm = DynamicPwm::allocate(PinName n);
  * pwm->setPeriod(1); // period now is 1ms
  * @endcode
  */
int DynamicPwm::setPeriod(int period)
{
    return setPeriodUs(period * 1000);
}
