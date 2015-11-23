#include "MicroBit.h"

#define MICROBIT_BLE_ENABLE_BONDING 	true
#define MICROBIT_BLE_REQUIRE_MITM		true


/*
 * Many of the mbed interfaces we need to use only support callbacks to plain C functions, rather than C++ methods.
 * So, we maintain a pointer to the MicroBitBLEManager that's in use. Ths way, we can still access resources on the micro:bit 
 * whilst keeping the code modular. 
 */
static MicroBitBLEManager *manager = NULL;

/**
  * Callback when a BLE GATT disconnect occurs.
  */
static void bleDisconnectionCallback(Gap::Handle_t handle, Gap::DisconnectionReason_t reason)
{
    (void) handle; /* -Wunused-param */
    (void) reason; /* -Wunused-param */

    if (manager)
	    manager->onDisconnectionCallback();

}

static void passkeyDisplayCallback(Gap::Handle_t handle, const SecurityManager::Passkey_t passkey)
{
    (void) handle; /* -Wunused-param */

	ManagedString passKey((const char *)passkey, SecurityManager::PASSKEY_LEN);

    if (manager)
	    manager->pairingRequested(passKey);
}

static void securitySetupCompletedCallback(Gap::Handle_t handle, SecurityManager::SecurityCompletionStatus_t status)
{
    (void) handle; /* -Wunused-param */

    if (manager)
	    manager->pairingComplete(status == SecurityManager::SEC_STATUS_SUCCESS);
}

/**
  * Constructor. 
  *
  * Configure and manage the micro:bit's Bluetooth Low Energy (BLE) stack.
  * Note that the BLE stack *cannot*  be brought up in a static context.
  * (the software simply hangs or corrupts itself).
  * Hence, we bring it up in an explicit init() method, rather than in the constructor.
  */
MicroBitBLEManager::MicroBitBLEManager() 
{   
	manager = this;
	this->ble = NULL;
	this->pairingStatus = 0;
}

/**
  * Method that is called whenever a BLE device disconnects from us.
  * The nordic stack stops dvertising whenever a device connects, so we use
  * this callback to restart advertising.
  */
void MicroBitBLEManager::onDisconnectionCallback()
{   
	if(ble)
    	ble->startAdvertising();  
}

/**
  * Post constructor initialisation method.
  * After *MUCH* pain, it's noted that the BLE stack can't be brought up in a 
  * static context, so we bring it up here rather than in the constructor.
  * n.b. This method *must* be called in main() or later, not before.
  *
  * Example:
  * @code 
  * bleManager.init("zevug");
  * @endcode
  */
void MicroBitBLEManager::init(ManagedString deviceName)
{   
	ManagedString prefix("BBC micro:bit [");
	ManagedString postfix("]");
	ManagedString BLEName = prefix + deviceName + postfix;

	this->deviceName = deviceName;

    // Start the BLE stack.        
    ble = new BLEDevice();
    ble->init();

    // automatically restart advertising after a device disconnects.
    ble->onDisconnection(bleDisconnectionCallback);

    // Setup our security requirements.
    ble->securityManager().onPasskeyDisplay(passkeyDisplayCallback);
    ble->securityManager().onSecuritySetupCompleted(securitySetupCompletedCallback);
    ble->securityManager().init(MICROBIT_BLE_ENABLE_BONDING, MICROBIT_BLE_REQUIRE_MITM, SecurityManager::IO_CAPS_DISPLAY_ONLY);

    // Configure for high speed mode where possible.
    Gap::ConnectionParams_t fast;
    ble->getPreferredConnectionParams(&fast);
    fast.minConnectionInterval = 8; // 10 ms
    fast.maxConnectionInterval = 16; // 20 ms
    fast.slaveLatency = 0;
    ble->setPreferredConnectionParams(&fast);

    // Setup advertising.
    ble->accumulateAdvertisingPayload(GapAdvertisingData::BREDR_NOT_SUPPORTED | GapAdvertisingData::LE_GENERAL_DISCOVERABLE);
    ble->accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LOCAL_NAME, (uint8_t *)BLEName.toCharArray(), BLEName.length());
    ble->setAdvertisingType(GapAdvertisingParams::ADV_CONNECTABLE_UNDIRECTED);
    ble->setAdvertisingInterval(Gap::MSEC_TO_ADVERTISEMENT_DURATION_UNITS(200));
    ble->startAdvertising();  
}

/**
 * A request to pair has been received from a BLE device.
 * If we're in BLUEZONE mode, display the passkey to the user.
 */
void MicroBitBLEManager::pairingRequested(ManagedString passKey)
{
	this->passKey = passKey;
	this->pairingStatus = MICROBIT_BLE_PAIR_REQUEST;
}

/**
 * A pairing request has been sucesfully completed.
 * If we're in BLUEZONE mode, display feedback to the user.
 */
void MicroBitBLEManager::pairingComplete(bool success)
{
	this->pairingStatus &= ~MICROBIT_BLE_PAIR_REQUEST;
	this->pairingStatus |= MICROBIT_BLE_PAIR_COMPLETE;

	if(success)
		this->pairingStatus |= MICROBIT_BLE_PAIR_SUCCESSFUL;
}

/**
 * Enter BLUEZONE mode. This is mode is called to initiate pairing, and to enable FOTA programming
 * of the micro:bit in cases where BLE is disabled during normal operation.
 */
void MicroBitBLEManager::bluezone(MicroBitDisplay &display)
{  
	ManagedString prefix("BLUEZONE:");
	ManagedString msg = prefix + deviceName;

	// Stop any running animations on the display
	display.stopAnimation();
	display.scroll(msg);

	// Display our name, visualised as a histogram in the display to aid identification.
	showNameHistogram(display);

	while(1)
	{
		if (pairingStatus & MICROBIT_BLE_PAIR_REQUEST)
		{
			display.scroll("Pair: ", 90);
			display.scroll(passKey, 90);
		}

		if (pairingStatus & MICROBIT_BLE_PAIR_COMPLETE)
		{
			if (pairingStatus & MICROBIT_BLE_PAIR_SUCCESSFUL)
			{
				MicroBitImage tick("0,0,0,0,0\n0,0,0,0,255\n0,0,0,255,0\n255,0,255,0,0\n0,255,0,0,0\n");
				display.print(tick,0,0,0);
			}
			else
			{
				MicroBitImage cross("255,0,0,0,255\n0,255,0,255,0\n0,0,255,0,0\n0,255,0,255,0\n255,0,0,0,255\n");
				display.print(cross,0,0,0);
			}
		}

		MicroBit::sleep(100);
	}
}

/**
  * Displays the device's ID code as a histogram on the LED matrix display.
  */
void MicroBitBLEManager::showNameHistogram(MicroBitDisplay &display)
{
    uint32_t n = NRF_FICR->DEVICEID[1];
    int ld = 1;
    int d = MICROBIT_DFU_HISTOGRAM_HEIGHT;
    int h;

    display.clear();
    for (int i=0; i<MICROBIT_DFU_HISTOGRAM_WIDTH;i++)
    {
        h = (n % d) / ld;

        n -= h;
        d *= MICROBIT_DFU_HISTOGRAM_HEIGHT;
        ld *= MICROBIT_DFU_HISTOGRAM_HEIGHT;

        for (int j=0; j<h+1; j++)
            display.image.setPixelValue(MICROBIT_DFU_HISTOGRAM_WIDTH-i-1, MICROBIT_DFU_HISTOGRAM_HEIGHT-j-1, 255);
    }       
}

