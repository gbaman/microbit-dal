#ifndef MICROBIT_H
#define MICROBIT_H

// DEBUG. Enable this to get debug message routed through the USB serial interface.
//#define MICROBIT_DBG

#include "mbed.h"
#include "ble/BLE.h"
#include "ble/services/DeviceInformationService.h"

//error number enumeration
#include "ErrorNo.h"

/**
  * Displays "=(" and an accompanying status code 
  * @param statusCode the appropriate status code - 0 means no code will be displayed. Status codes must be in the range 0-255.
  */
void panic(int statusCode);

#include "MicroBitMalloc.h"
#include "MicroBitCompat.h"
#include "MicroBitFiber.h"
#include "ManagedType.h"
#include "ManagedString.h"
#include "MicroBitFont.h"
#include "MicroBitImage.h"
#include "MicroBitEvent.h"
#include "MicroBitMessageBus.h"
#include "DynamicPwm.h"
#include "MicroBitComponent.h"
#include "MicroBitI2C.h"
#include "MicroBitSerial.h"
#include "MicroBitButton.h"
#include "MicroBitMultiButton.h"
#include "MicroBitDisplay.h"
#include "MicroBitPin.h"
#include "MicroBitIO.h"
#include "MicroBitCompass.h"
#include "MicroBitAccelerometer.h"

#include "MicroBitDFUService.h"
#include "MicroBitEventService.h"
#include "ExternalEvents.h"

// MicroBit::flags values
#define MICROBIT_FLAG_SCHEDULER_RUNNING         0x00000001
#define MICROBIT_FLAG_ACCELEROMETER_RUNNING     0x00000002
#define MICROBIT_FLAG_DISPLAY_RUNNING           0x00000004
#define MICROBIT_FLAG_COMPASS_RUNNING           0x00000008


// Random number generator
#define NRF51822_RNG_ADDRESS            0x4000D000

#define MICROBIT_IO_PINS                20            // TODO: Need to change for live, currently 3 for test

// Enumeration of core components.
#define MICROBIT_ID_BUTTON_A            1
#define MICROBIT_ID_BUTTON_B            2
#define MICROBIT_ID_BUTTON_RESET        3
#define MICROBIT_ID_ACCELEROMETER       4
#define MICROBIT_ID_COMPASS             5
#define MICROBIT_ID_DISPLAY             6

//EDGE connector events
#define MICROBIT_ID_IO_P0               7           //P0 is the left most pad (ANALOG/DIGITAL) 
#define MICROBIT_ID_IO_P1               8           //P1 is the middle pad (ANALOG/DIGITAL) 
#define MICROBIT_ID_IO_P2               9           //P2 is the right most pad (ANALOG/DIGITAL) 
#define MICROBIT_ID_IO_P3               10           //COL1 (ANALOG/DIGITAL) 
#define MICROBIT_ID_IO_P4               11          //BTN_A        
#define MICROBIT_ID_IO_P5               12          //COL2 (ANALOG/DIGITAL) 
#define MICROBIT_ID_IO_P6               13          //ROW2
#define MICROBIT_ID_IO_P7               14          //ROW1       
#define MICROBIT_ID_IO_P8               15          //PIN 18
#define MICROBIT_ID_IO_P9               16          //ROW3                  
#define MICROBIT_ID_IO_P10              17          //COL3 (ANALOG/DIGITAL) 
#define MICROBIT_ID_IO_P11              18          //BTN_B
#define MICROBIT_ID_IO_P12              19          //PIN 20
#define MICROBIT_ID_IO_P13              20          //SCK
#define MICROBIT_ID_IO_P14              21          //MISO
#define MICROBIT_ID_IO_P15              22          //MOSI
#define MICROBIT_ID_IO_P16              23          //PIN 16
#define MICROBIT_ID_IO_P19              24          //SCL
#define MICROBIT_ID_IO_P20              25          //SDA

#define MICROBIT_ID_BUTTON_AB           26          // Button A+B multibutton

// mBed pin assignments of core components.
//TODO: When platform is built for MB2 - pins will be defined by default, these will change...
#define MICROBIT_PIN_SDA                P0_30
#define MICROBIT_PIN_SCL                P0_0

#define MICROBIT_SYSTEM_COMPONENTS      10
#define MICROBIT_IDLE_COMPONENTS        6

#ifdef MICROBIT_DEBUG
extern Serial pc;
#endif

/**
  * Class definition for a MicroBit device.
  *
  * Represents the device as a whole, and includes member variables to that reflect the components of the system.
  */
class MicroBit
{                                  
    private:
    
    void                    seedRandom();
    uint32_t                randomValue;
  
    public:
    
    // Map of device state.
    uint32_t                flags;

    // Periodic callback
    Ticker                  systemTicker;

    // I2C Interface
    MicroBitI2C             i2c;  
    
    // Serial Interface
    MicroBitSerial          serial;   

    // Array of components which are iterated during a system tick
    MicroBitComponent*      systemTickComponents[MICROBIT_SYSTEM_COMPONENTS];
    
    // Array of components which are iterated during idle thread execution, isIdleCallbackNeeded is polled during a systemTick.
    MicroBitComponent*      idleThreadComponents[MICROBIT_IDLE_COMPONENTS];

    // Device level Message Bus abstraction
    MicroBitMessageBus      MessageBus;     
    
    // Member variables to represent each of the core components on the device.
    MicroBitDisplay         display;
    MicroBitButton          buttonA;
    MicroBitButton          buttonB;
    MicroBitMultiButton     buttonAB;    
    MicroBitAccelerometer   accelerometer;
    MicroBitCompass         compass;

    //An object of available IO pins on the device
    MicroBitIO              io;
    
    // Bluetooth related member variables.
    BLEDevice                   *ble;
    DeviceInformationService    *ble_device_information_service;
    MicroBitDFUService          *ble_firmware_update_service;
    MicroBitEventService        *ble_event_service;

    
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
    MicroBit();  

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
    void init();

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
    void sleep(int milliseconds);

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
    int random(int max);

    /**
      * Period callback. Used by MicroBitDisplay, FiberScheduler and I2C sensors to
      * provide a power efficient sense of time.
      */
    void systemTick();
    
    /**
      * System tasks to be executed by the idle thread when the Micro:Bit isn't busy or when data needs to be read.
      */
    void systemTasks();

    /**
      * add a component to the array of system components which invocate the systemTick member function during a systemTick 
      */
    void addSystemComponent(MicroBitComponent *component);
    
    /**
      * remove a component from the array of system components
      */
    void removeSystemComponent(MicroBitComponent *component);
    
    /**
      * add a component to the array of of idle thread components.
      * isIdleCallbackNeeded is polled during a systemTick to determine if the idle thread should jump to the front of the queue
      */
    void addIdleComponent(MicroBitComponent *component);
    
    /**
      * remove a component from the array of idle thread components
      */
    void removeIdleComponent(MicroBitComponent *component);

    /**
      * Determine the time since this MicroBit was last reset.
      *
      * @return The time since the last reset, in milliseconds. This will result in overflow after 1.6 months.
      * TODO: handle overflow case.
      */
    unsigned long systemTime();
    
    /**
      * Triggers a microbit panic where an infinite loop will occur swapping between the panicFace and statusCode if provided.
      * 
      * @param statusCode the status code of the associated error. Status codes must be in the range 0-255.
      */
    void panic(int statusCode = 0);

};

// Definition of the global instance of the MicroBit class.
// Using this as a variation on the singleton pattern, just to make
// code integration a little bit easier for 3rd parties.
extern MicroBit uBit;

// Entry point for application programs. Called after the super-main function
// has initialized the device and runtime environment.
extern "C" void app_main();


#endif

