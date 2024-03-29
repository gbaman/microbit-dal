/**
  * Class definition for the custom MicroBit LED Service.
  * Provides a BLE service to remotely read and write the state of the LED display.
  */
  
#include "MicroBit.h"
#include "ble/UUID.h"

#include "MicroBitLEDService.h"

/**
  * Constructor. 
  * Create a representation of the LEDService
  * @param _ble The instance of a BLE device that we're running on.
  */
MicroBitLEDService::MicroBitLEDService(BLEDevice &_ble) : 
        ble(_ble), 
        matrixCharacteristic(MicroBitLEDServiceMatrixUUID, (uint8_t *)&matrixCharacteristicBuffer, 0, sizeof(matrixCharacteristicBuffer), 
    GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ)
{
    // Create the data structures that represent each of our characteristics in Soft Device.
    GattCharacteristic  textCharacteristic(MicroBitLEDServiceTextUUID, (uint8_t *)textCharacteristicBuffer, 0, MICROBIT_BLE_MAXIMUM_SCROLLTEXT, 
    GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE);

    GattCharacteristic  scrollingSpeedCharacteristic(MicroBitLEDServiceScrollingSpeedUUID, (uint8_t *)&scrollingSpeedCharacteristicBuffer, 0, 
    sizeof(scrollingSpeedCharacteristicBuffer), GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ);

    // Initialise our characteristic values.
    memclr(matrixCharacteristicBuffer, sizeof(matrixCharacteristicBuffer));
    textCharacteristicBuffer[0] = 0;
    scrollingSpeedCharacteristicBuffer = MICROBIT_DEFAULT_SCROLL_SPEED;
    
    matrixCharacteristic.setReadAuthorizationCallback(this, &MicroBitLEDService::onDataRead);
            
    // Set default security requirements
    matrixCharacteristic.requireSecurity(SecurityManager::SECURITY_MODE_ENCRYPTION_WITH_MITM);
    textCharacteristic.requireSecurity(SecurityManager::SECURITY_MODE_ENCRYPTION_WITH_MITM);
    scrollingSpeedCharacteristic.requireSecurity(SecurityManager::SECURITY_MODE_ENCRYPTION_WITH_MITM);

    GattCharacteristic *characteristics[] = {&matrixCharacteristic, &textCharacteristic, &scrollingSpeedCharacteristic};
    GattService         service(MicroBitLEDServiceUUID, characteristics, sizeof(characteristics) / sizeof(GattCharacteristic *));

    ble.addService(service);

    matrixCharacteristicHandle = matrixCharacteristic.getValueHandle();
    textCharacteristicHandle = textCharacteristic.getValueHandle();
    scrollingSpeedCharacteristicHandle = scrollingSpeedCharacteristic.getValueHandle();

    ble.gattServer().write(scrollingSpeedCharacteristicHandle, (const uint8_t *)&scrollingSpeedCharacteristicBuffer, sizeof(scrollingSpeedCharacteristicBuffer));
    ble.gattServer().write(matrixCharacteristicHandle, (const uint8_t *)&matrixCharacteristicBuffer, sizeof(matrixCharacteristicBuffer));

    ble.onDataWritten(this, &MicroBitLEDService::onDataWritten);
}


/**
  * Callback. Invoked when any of our attributes are written via BLE.
  */
void MicroBitLEDService::onDataWritten(const GattWriteCallbackParams *params)
{   
    uint8_t *data = (uint8_t *)params->data;

    if (params->handle == matrixCharacteristicHandle && params->len > 0 && params->len < 6)
    {
        for (int y=0; y<params->len; y++)        
            for (int x=0; x<5; x++)        
                uBit.display.image.setPixelValue(x, y, (data[y] & (0x01 << (4-x))) ? 255 : 0);
    }

    else if (params->handle == textCharacteristicHandle) 
    {
        // Create a ManagedString representation from the UTF8 data.
        // We do this explicitly to control the length (in case the string is not NULL terminated!)
        ManagedString s((char *)params->data, params->len);

        // Start the string scrolling and we're done.
        uBit.display.scrollAsync(s, (int) scrollingSpeedCharacteristicBuffer);
    }

    else if (params->handle == scrollingSpeedCharacteristicHandle && params->len >= sizeof(scrollingSpeedCharacteristicBuffer)) 
    {
        // Read the speed requested, and store it locally.
        // We use this as the speed for all scroll operations subsquently initiated from BLE.
        scrollingSpeedCharacteristicBuffer = *((uint16_t *)params->data);
    }
}

/**
  * Callback. Invoked when any of our attributes are read via BLE.
  */
void MicroBitLEDService::onDataRead(GattReadAuthCallbackParams *params)
{
    if (params->handle == matrixCharacteristicHandle)
    {
        for (int y=0; y<5; y++)        
        {
            matrixCharacteristicBuffer[y] = 0;

            for (int x=0; x<5; x++)        
            {
                if (uBit.display.image.getPixelValue(x, y))
                    matrixCharacteristicBuffer[y] |= 0x01 << (4-x);
            }
        }

        ble.gattServer().write(matrixCharacteristicHandle, (const uint8_t *)&matrixCharacteristicBuffer, sizeof(matrixCharacteristicBuffer));
    }
}


const uint8_t  MicroBitLEDServiceUUID[] = {
    0xe9,0x5d,0xd9,0x1d,0x25,0x1d,0x47,0x0a,0xa0,0x62,0xfa,0x19,0x22,0xdf,0xa9,0xa8
};

const uint8_t  MicroBitLEDServiceMatrixUUID[] = {
    0xe9,0x5d,0x7b,0x77,0x25,0x1d,0x47,0x0a,0xa0,0x62,0xfa,0x19,0x22,0xdf,0xa9,0xa8
};

const uint8_t  MicroBitLEDServiceTextUUID[] = {
    0xe9,0x5d,0x93,0xee,0x25,0x1d,0x47,0x0a,0xa0,0x62,0xfa,0x19,0x22,0xdf,0xa9,0xa8
};

const uint8_t  MicroBitLEDServiceScrollingSpeedUUID[] = {
    0xe9,0x5d,0x0d,0x2d,0x25,0x1d,0x47,0x0a,0xa0,0x62,0xfa,0x19,0x22,0xdf,0xa9,0xa8
};


