#include "inc/MicroBit.h"

/**
  * Constructor. 
  * Create a compass representation with the given ID.
  * @param id the ID of the new object.
  * @param address the I2C address of the device.
  */
MicroBitCompass::MicroBitCompass(int id, int address) : average(), sample(), int1(MICROBIT_PIN_COMPASS_DATA_READY)
{
    this->id = id;
    this->address = address;

    // Enable automatic reset after each sample;
    writeCommand(MAG_CTRL_REG2, 0x80);
    
    // Select 10Hz update rate, with oversampling. Also enables the device.
    writeCommand(MAG_CTRL_REG1, 0x61);    
}

/**
  * Issues a standard, 2 byte I2C command write to the magnetometer.
  * Blocks the calling thread until complete.
  *
  * @param reg The address of the register to write to.
  * @param value The value to write.
  */
void MicroBitCompass::writeCommand(uint8_t reg, uint8_t value)
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
void MicroBitCompass::readCommand(uint8_t reg, uint8_t* buffer, int length)
{
    uBit.i2c.write(address, (const char *)&reg, 1, true);
    uBit.i2c.read(address, (char *)buffer, length);
}


/**
  * Issues a read of a given address, and returns the value.
  * Blocks the calling thread until complete.
  *
  * @param reg The based address of the 16 bit register to access.
  * @return The register value, interpreted as a 16 but signed value.
  */
int16_t MicroBitCompass::read16(uint8_t reg)
{
    uint8_t cmd[2];

    cmd[0] = reg;
    uBit.i2c.write(address, (const char *)cmd, 1);

    cmd[0] = 0x00;
    cmd[1] = 0x00;
    
    uBit.i2c.read(address, (char *)cmd, 2);
    return (int16_t)( (cmd[1]|(cmd[0] << 8))); //concatenate the MSB and LSB
}

/**
  * Gets the current heading of the device, relative to magnetic north.
  * @return the current heading, in degrees.
  */
int MicroBitCompass::heading()
{
    return (atan2((double)(sample.y - average.y),(double)(sample.x - average.x)))*180/PI;
}


/**
  * Periodic callback from MicroBit clock.
  * Check if any data is ready for reading...
  */
void MicroBitCompass::tick()
{
    // Poll interrupt line from accelerometer.
    // Active HI. Interrupt is cleared in data read of MAG_OUT_X_MSB.
    if(int1)
    {
        sample.x = read16(MAG_OUT_X_MSB);
        sample.y = read16(MAG_OUT_Y_MSB);
        sample.z = read16(MAG_OUT_Z_MSB);
    }
}

/**
  * Reads the X axis value of the latest update from the accelerometer.
  * @return The force measured in the X axis, in milli-g.
  */
int MicroBitCompass::getX()
{
    return sample.x;
}

/**
  * Reads the Y axis value of the latest update from the accelerometer.
  * @return The force measured in the Y axis, in milli-g.
  */    
int MicroBitCompass::getY()
{
    return sample.y;
}

/**
  * Reads the Z axis value of the latest update from the accelerometer.
  * @return The force measured in the Z axis, in milli-g.
  */    
int MicroBitCompass::getZ()
{
    return sample.z;
}
