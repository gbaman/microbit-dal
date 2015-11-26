#include "MicroBit.h"

MicroBit        uBit;
InterruptIn     resetButton(MICROBIT_PIN_BUTTON_RESET);

extern char* MICROBIT_BLE_DEVICE_NAME;

int main()
{    
    // Bring up soft reset button.
    resetButton.mode(PullUp);
    resetButton.fall(microbit_reset);
    
#if CONFIG_ENABLED(MICROBIT_DBG)

    // For diagnostics. Gives time to open the console window. :-) 
    for (int i=3; i>0; i--)
    {
        uBit.serial.printf("=== SUPERMAIN: Starting in %d ===\n", i);
        wait(1.0);
    }

    uBit.serial.printf("micro:bit runtime DAL version %s\n", MICROBIT_DAL_VERSION);
#endif    

    // Bring up our nested heap allocator.
    microbit_heap_init();

    // Bring up fiber scheduler
    uBit.serial.printf("Sched init\n");
    scheduler_init();
    uBit.serial.printf("Sched init done\n");
    
    // Bring up random number generator, BLE, display and system timers.    
    uBit.serial.printf("uBit init\n");
    uBit.init();
    uBit.serial.printf("uBit init done\n");

    // Provide time for all threaded initialisers to complete.
    uBit.sleep(100);

#if CONFIG_ENABLED(MICROBIT_BLE_BLUEZONE)
    // Test if we need to enter BLE pairing mode...
    int i=0;
    while (uBit.buttonA.isPressed() && uBit.buttonB.isPressed() && i<10)
    {
        uBit.sleep(100);
        i++;
        
        if (i == 10)
        {
			// Bring up the BLE stack if it isn't alredy done.
			if (!uBit.ble)
				uBit.bleManager.init(uBit.getName(), uBit.getSerial());

            // Enter BLUE ZONE mode, using the LED matrix for any necessary paiing operations
			uBit.bleManager.bluezone(uBit.display);

        }
    }
#endif
       
    uBit.serial.printf("app_main\n");
    app_main();

    // If app_main exits, there may still be other fibers running, registered event handlers etc.
    // Simply release this fiber, which will mean we enter the scheduler. Worse case, we then
    // sit in the idle task forever, in a power efficient sleep.
    release_fiber();
   
    // We should never get here, but just in case.
    while(1);
}
