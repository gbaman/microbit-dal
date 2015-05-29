/**
  * Class definition for MicroBit Accelerometer.
  *
  * Represents an implementation of the Freescale MMA8653 3 axis accelerometer
  * Also includes basic data caching and on demand activation.
  */
  
#include "MicroBit.h"

/**
  * Issues a standard, 2 byte I2C command write to the accelerometer.
  * Blocks the calling thread until complete.
  *
  * @param reg The address of the register to write to.
  * @param value The value to write.
  */
void MicroBitAccelerometer::writeCommand(uint8_t reg, uint8_t value)
{
    uint8_t command[2];
    command[0] = reg;
    command[1] = value;
    
    uBit.i2c.write(address, (const char *)command, 2);
}

/**
  * Issues a read command into the specified buffer.
  * Blocks the calling thread until complete.
  *
  * @param reg The address of the register to access.
  * @param buffer Memory area to read the data into.
  * @param length The number of bytes to read.
  */
void MicroBitAccelerometer::readCommand(uint8_t reg, uint8_t* buffer, int length)
{
    uBit.i2c.write(address, (const char *)&reg, 1, true);
    uBit.i2c.read(address, (char *)buffer, length);
}

MicroBitAccelerometer::MicroBitAccelerometer(int id, int address) : sample(), int1(MICROBIT_PIN_ACCEL_DATA_READY)
{
    // Store our identifiers.
    this->id = id;
    this->address = address;

    // Enable the accelerometer.
    // First place the device into standby mode, so it can be configured.
    writeCommand(MMA8653_CTRL_REG1, 0x00);
    
    // Enable the INT1 interrupt pin.
    writeCommand(MMA8653_CTRL_REG4, 0x01);

    // Select the DATA_READY event source to be routed to INT1
    writeCommand(MMA8653_CTRL_REG5, 0x01);

    // Configure for a +/- 2g range.
    writeCommand(MMA8653_XYZ_DATA_CFG, 0x00);
    
    // Bring the device back online, with 10bit wide samples at a 50Hz frequency.
    writeCommand(MMA8653_CTRL_REG1, 0x21);

    // indicate that we're ready to receive tick callbacks.
    uBit.flags |= MICROBIT_FLAG_ACCELEROMETER_RUNNING;
}

int MicroBitAccelerometer::whoAmI()
{
    uint8_t data;

    readCommand(MMA8653_WHOAMI, &data, 1);    
    return (int)data;
}

/**
  * Reads the acceleration data from the accelerometer, and stores it in our buffer.
  */
void MicroBitAccelerometer::update()
{
    int8_t data[6];

    readCommand(MMA8653_OUT_X_MSB, (uint8_t *)data, 6);

    // read MSB values...
    sample.x = data[0]; 
    sample.y = data[2];
    sample.z = data[4];
    
    // Scale into millig (approx!)
    sample.x *= 16;
    sample.y *= 16;
    sample.z *= 16;

    // We ignore the LSB bits for now, as they're just noise...
    // TODO: Revist this when we have working samples to see if additional resolution is needed.

#ifdef USE_ACCEL_LSB
    // Add in LSB values.
    sample.x += (data[1] / 64);
    sample.y += (data[3] / 64);
    sample.z += (data[5] / 64);
#endif

    //TODO: Issue an event.
};

/**
  * Reads the X axis value of the latest update from the accelerometer.
  * @return The force measured in the X axis, in milli-g.
  */
int MicroBitAccelerometer::getX()
{
    return sample.x;
}

/**
  * Reads the Y axis value of the latest update from the accelerometer.
  * @return The force measured in the Y axis, in milli-g.
  */    
int MicroBitAccelerometer::getY()
{
    return sample.y;
}

/**
  * Reads the Z axis value of the latest update from the accelerometer.
  * @return The force measured in the Z axis, in milli-g.
  */    
int MicroBitAccelerometer::getZ()
{
    return sample.z;
}


/**
  * periodic callback from MicroBit clock.
  * Check if any data is ready for reading...
  */    
void MicroBitAccelerometer::tick()
{
    // Poll interrupt line from accelerometer.
    // n.b. Default is Active LO. Interrupt is cleared in data read.
    if(!int1)
        update();
}