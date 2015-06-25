#include "MicroBit.h"

char MICROBIT_BLE_DEVICE_NAME[] = "BBC MicroBit [xxxxx]";
char MICROBIT_BLE_MANUFACTURER[] = "The Cast of W1A";
char MICROBIT_BLE_MODEL[] = "Microbit SB2";
char MICROBIT_BLE_SERIAL[] = "SN1";
char MICROBIT_BLE_HARDWARE_VERSION[] = "0.2";
char MICROBIT_BLE_FIRMWARE_VERSION[] = "1.1";
char MICROBIT_BLE_SOFTWARE_VERSION[] = "1.0";

/**
  * Constructor. 
  * Create a representation of a MicroBit device as a global singleton.
  * @param messageBus callback function to receive MicroBitMessageBus events.
  *
  * Exposed objects:
  * @code 
  * uBit.systemTicker; //the Ticker callback that performs routines like updating the display.
  * uBit.MessageBus; //The message bus where events are fired.
  * uBit.display; //The display object for the LED matrix.
  * uBit.buttonA; //The buttonA object for button a.
  * uBit.buttonB; //The buttonB object for button b.
  * uBit.resetButton; //The resetButton used for soft resets.
  * uBit.accelerometer; //The object that represents the inbuilt accelerometer
  * uBit.compass; //The object that represents the inbuilt compass(magnetometer)
  * uBit.io.P*; //Where P* is P0 to P16, P19 & P20 on the edge connector
  * @endcode
  */
MicroBit::MicroBit() : 
    flags(0x00),
    i2c(MICROBIT_PIN_SDA, MICROBIT_PIN_SCL),
    MessageBus(),
    display(MICROBIT_ID_DISPLAY, 5, 5),
    buttonA(MICROBIT_ID_BUTTON_A,MICROBIT_PIN_BUTTON_A),
    buttonB(MICROBIT_ID_BUTTON_B,MICROBIT_PIN_BUTTON_B),
    resetButton(MICROBIT_ID_BUTTON_RESET,MICROBIT_PIN_BUTTON_RESET),
    accelerometer(MICROBIT_ID_ACCELEROMETER, MMA8653_DEFAULT_ADDR),
    compass(MICROBIT_ID_COMPASS, MAG3110_DEFAULT_ADDR),
    io(MICROBIT_ID_IO_P0,MICROBIT_ID_IO_P1,MICROBIT_ID_IO_P2,
       MICROBIT_ID_IO_P3,MICROBIT_ID_IO_P4,MICROBIT_ID_IO_P5,
       MICROBIT_ID_IO_P6,MICROBIT_ID_IO_P7,MICROBIT_ID_IO_P8,
       MICROBIT_ID_IO_P9,MICROBIT_ID_IO_P10,MICROBIT_ID_IO_P11,
       MICROBIT_ID_IO_P12,MICROBIT_ID_IO_P13,MICROBIT_ID_IO_P14,
       MICROBIT_ID_IO_P15,MICROBIT_ID_IO_P16,MICROBIT_ID_IO_P19,
       MICROBIT_ID_IO_P20)
{   
}

/**
  * Post constructor initialisation method.
  * After *MUCH* pain, it's noted that the BLE stack can't be brought up in a 
  * static context, so we bring it up here rather than in the constructor.
  * n.b. This method *must* be called in main() or later, not before.
  *
  * Example:
  * @code 
  * uBit.init();
  * @endcode
  */
void MicroBit::init()
{   

#ifdef MICROBIT_BLE
    // Start the BLE stack.        
    ble = new BLEDevice();
    
    ble->init();
 
    // Add our auxiliary services.
    ble_firmware_update_service = new MicroBitDFUService(*ble);
    ble_device_information_service = new DeviceInformationService(*ble, MICROBIT_BLE_MANUFACTURER, MICROBIT_BLE_MODEL, MICROBIT_BLE_SERIAL, MICROBIT_BLE_HARDWARE_VERSION, MICROBIT_BLE_FIRMWARE_VERSION, MICROBIT_BLE_SOFTWARE_VERSION);
    ble_event_service = new MicroBitEventService(*ble);
    
    // Compute our auto-generated MicroBit device name.
    ble_firmware_update_service->getName(MICROBIT_BLE_DEVICE_NAME+14);
    
    // Setup advertising.
    ble->accumulateAdvertisingPayload(GapAdvertisingData::BREDR_NOT_SUPPORTED | GapAdvertisingData::LE_GENERAL_DISCOVERABLE);
    ble->accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LOCAL_NAME, (uint8_t *)MICROBIT_BLE_DEVICE_NAME, sizeof(MICROBIT_BLE_DEVICE_NAME));
    ble->setAdvertisingType(GapAdvertisingParams::ADV_CONNECTABLE_UNDIRECTED);
    ble->setAdvertisingInterval(Gap::MSEC_TO_ADVERTISEMENT_DURATION_UNITS(1000));
    ble->startAdvertising();   

#else 

    ble = NULL;
    ble_firmware_update_service = NULL;
    ble_device_information_service = NULL;
    ble_event_service = NULL;
    
#endif

    // Start refreshing the Matrix Display
    systemTicker.attach(this, &MicroBit::systemTick, MICROBIT_DISPLAY_REFRESH_PERIOD);     
}

/**
  * Delay for the given amount of time.
  * If the scheduler is running, this will deschedule the current fiber and perform
  * a power efficent, concurrent sleep operation.
  * If the scheduler is disabled or we're running in an interrupt context, this
  * will revert to a busy wait. 
  *
  * @note Values of 6 and below tend to lose resolution - do you really need to sleep for this short amount of time?
  * 
  * @param milliseconds the amount of time, in ms, to wait for. This number cannot be negative.
  *
  * Example:
  * @code 
  * uBit.sleep(20); //sleep for 20ms
  * @endcode
  */
void MicroBit::sleep(int milliseconds)
{
    //sanity check, we can't time travel... (yet?)
    if(milliseconds < 0)
        return;
        
    if (flags & MICROBIT_FLAG_SCHEDULER_RUNNING)
        fiber_sleep(milliseconds);
    else
        wait_ms(milliseconds);
}

/**
  * Generate a random number in the given range.
  * We use the NRF51822 in built random number generator here
  * TODO: Determine if we want to, given its relatively high power consumption!
  *
  * @param max the upper range to generate a number for. This number cannot be negative
  * @return A random, natural number between 0 and the max-1. Or MICROBIT_INVALID_VALUE (defined in ErrorNo.h) if max is <= 0.
  *
  * Example:
  * @code 
  * uBit.random(200); //a number between 0 and 199
  * @endcode
  */
int MicroBit::random(int max)
{
    //return MICROBIT_INVALID_VALUE if max is <= 0...
    if(max <= 0)
        return MICROBIT_INVALID_VALUE;
        
    int retVal = 0;
        
    // Start the Random number generator. No need to leave it running... I hope. :-)
    NRF_RNG->TASKS_START = 1;
    
    for(int i = 0; i< 4 ;i++)
    {
        // Clear the VALRDY EVENT
        NRF_RNG->EVENTS_VALRDY = 0;
        
        // Wait for a number ot be generated.
        while ( NRF_RNG->EVENTS_VALRDY == 0);
        
        retVal += (int) NRF_RNG->VALUE;
    }
    // Disable the generator to save power.
    NRF_RNG->TASKS_STOP = 1;
    
    // Set output according to the random value
    return (retVal) % max;
}


/**
  * Period callback. Used by MicroBitDisplay, FiberScheduler and I2C sensors to
  * provide a power efficient sense of time.
  */
void MicroBit::systemTick()
{   
    // Refresh the matrix display, and update animations, if we need to.
    if (uBit.flags & MICROBIT_FLAG_DISPLAY_RUNNING)
        uBit.display.strobeUpdate();
                
    // Update Accelerometer if needed.
    if (uBit.flags & MICROBIT_FLAG_ACCELEROMETER_RUNNING)
        uBit.accelerometer.tick();
 
    // Update Accelerometer if needed.
    if (uBit.flags & MICROBIT_FLAG_COMPASS_RUNNING)
        uBit.compass.tick();
 
    // Scheduler callback. We do this here just as a single timer is more efficient. :-)
    if (uBit.flags & MICROBIT_FLAG_SCHEDULER_RUNNING)
        scheduler_tick();  
        
    //update the buttons if required.
    uBit.buttonA.tick();
    uBit.buttonB.tick();
}

/**
  * Determine the time since this MicroBit was last reset.
  *
  * @return The time since the last reset, in milliseconds. This will result in overflow after 1.6 months.
  * TODO: handle overflow case.
  */
unsigned long MicroBit::systemTime()
{
    return ticks;
}

/**
  * Triggers a microbit panic where an infinite loop will occur swapping between the panicFace and statusCode if provided.
  * 
  * @param statusCode the status code of the associated error. Status codes must be in the range 0-255.
  */
void MicroBit::panic(int statusCode)
{
    //show error and enter infinite while
    uBit.display.error(statusCode);
}

/**
  * custom function for panic for malloc & new due to scoping issue.
  */
void panic(int statusCode)
{
    uBit.panic(statusCode);   
}