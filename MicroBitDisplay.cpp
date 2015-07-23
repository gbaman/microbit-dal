/**
  * Class definition for a MicroBitDisplay.
  *
  * A MicroBitDisplay represents the LED matrix array on the MicroBit device.
  */
#include "MicroBit.h"
#include "MicroBitMatrixMaps.h"
#include <new>
#include "nrf_gpio.h"
#include "mbed.h"

/**
  * Constructor.
  * Create a Point representation of an LED on a matrix
  * Used to handle non-linear matrix layouts.
  */
MatrixPoint::MatrixPoint(uint8_t x, uint8_t y)
{
    this->x = x;
    this->y = y;
}

/**
  * Constructor.
  * Create a representation of a display of a given size.
  * The display is initially blank.
  *
  * @param x the width of the display in pixels.
  * @param y the height of the display in pixels.
  * 
  * Example:
  * @code 
  * MicroBitDisplay display(MICROBIT_ID_DISPLAY, 5, 5),
  * @endcode
  */
MicroBitDisplay::MicroBitDisplay(uint16_t id, uint8_t x, uint8_t y) : 
    columnPins(MICROBIT_DISPLAY_COLUMN_PINS), 
    font(),
    image(x*2,y)
{
    this->rowDrive = DynamicPwm::allocate(rowPins[0],PWM_PERSISTENCE_PERSISTENT);
    this->id = id;
    this->width = x;
    this->height = y;
    this->strobeRow = 0;
    this->rowDrive->period_us(MICROBIT_DISPLAY_PWM_PERIOD);
    
    this->rotation = MICROBIT_DISPLAY_ROTATION_0;
    this->setBrightness(MICROBIT_DEFAULT_BRIGHTNESS);
    
    animationMode = ANIMATION_MODE_NONE;
    
    uBit.flags |= MICROBIT_FLAG_DISPLAY_RUNNING;
}

/**
  * Internal frame update method, used to strobe the display.
  *
  * TODO: Write a more efficient, complementary variation of this method for the case where 
  * MICROBIT_DISPLAY_ROW_COUNT > MICROBIT_DISPLAY_COLUMN_COUNT.
  */   
void MicroBitDisplay::systemTick()
{   
    // TODO: Cache coldata for future use, so we don't recompute so often?
    int coldata;

    // Move on to the next row.    
    strobeRow = (strobeRow+1) % MICROBIT_DISPLAY_ROW_COUNT;
        
    // Calculate the bitpattern to write.
    coldata = 0;
    for (int i = 0; i<MICROBIT_DISPLAY_COLUMN_COUNT; i++)
    {
        int x = matrixMap[i][strobeRow].x;
        int y = matrixMap[i][strobeRow].y;
        int t = x;        
        
        switch (rotation)
        {
            case MICROBIT_DISPLAY_ROTATION_90:
                x = width - 1 - y;
                y = t;
                break;
                
            case MICROBIT_DISPLAY_ROTATION_180:
                x = width - 1 - x;
                y = height - 1 - y;
                break;
                
            case MICROBIT_DISPLAY_ROTATION_270:
                x = y;
                y = height - 1 - t;
                break;
        }
        
        if(image.getPixelValue(x, y))
            coldata |= (1 << i);
    }

    // Write to the matrix display.
    columnPins.write(0xffff);    

    rowDrive->redirect(rowPins[strobeRow]);

    columnPins.write(~coldata);

    // Update text and image animations if we need to.
    this->animationUpdate();
}

/**
  * Periodic callback, that we use to perform any animations we have running.
  */
void
MicroBitDisplay::animationUpdate()
{   
    if (animationMode == ANIMATION_MODE_NONE)
        return;
    
    animationTick += FIBER_TICK_PERIOD_MS; 
    
    if(animationTick >= animationDelay)
    {
        animationTick = 0;
        
        if (animationMode == ANIMATION_MODE_SCROLL_TEXT)
            this->updateScrollText();
        
        if (animationMode == ANIMATION_MODE_PRINT_TEXT)
            this->updatePrintText();

        if (animationMode == ANIMATION_MODE_SCROLL_IMAGE)
            this->updateScrollImage();
            
        if (animationMode == ANIMATION_MODE_ANIMATE_IMAGE)
            this->updateAnimateImage();
            
    }
}

/**
  * Broadcasts an event onto the shared MessageBus
  * @param eventCode The ID of the event that has occurred.
  */
void MicroBitDisplay::sendEvent(uint16_t eventCode)
{
    MicroBitEvent evt(id,eventCode);
}

/**
  * Internal scrollText update method. 
  * Shift the screen image by one pixel to the left. If necessary, paste in the next char.
  */   
void MicroBitDisplay::updateScrollText()
{    
    image.shiftLeft(1);
    scrollingPosition++;
    
    if (scrollingPosition == width + MICROBIT_DISPLAY_SPACING)
    {        
        scrollingPosition = 0;
        
        image.print(scrollingChar < scrollingText.length() ? scrollingText.charAt(scrollingChar) : ' ',width,0);

        if (scrollingChar > scrollingText.length())
        {
            animationMode = ANIMATION_MODE_NONE;
            this->sendEvent(MICROBIT_DISPLAY_EVT_ANIMATION_COMPLETE);
            return;
        }
        scrollingChar++;
   }
}

/**
  * Internal printText update method. 
  * Paste in the next char in the string.
  */   
void MicroBitDisplay::updatePrintText()
{        
    image.print(printingChar < printingText.length() ? printingText.charAt(printingChar) : ' ',0,0);

    if (printingChar > printingText.length())
    {
        animationMode = ANIMATION_MODE_NONE;   
        this->sendEvent(MICROBIT_DISPLAY_EVT_ANIMATION_COMPLETE);
        return;
    }
    
    printingChar++;
}

/**
  * Internal scrollImage update method. 
  * Paste the stored bitmap at the appropriate point.
  */   
void MicroBitDisplay::updateScrollImage()
{   
    image.clear();     

    if ((image.paste(scrollingImage, scrollingImagePosition, 0, 0) == 0) && scrollingImageRendered)
    {
        animationMode = ANIMATION_MODE_NONE;  
        this->sendEvent(MICROBIT_DISPLAY_EVT_ANIMATION_COMPLETE);     
        return;
    }

    scrollingImagePosition += scrollingImageStride;
    scrollingImageRendered = true;
}


/**
  * Internal animateImage update method. 
  * Paste the stored bitmap at the appropriate point and stop on the last frame.
  */   
void MicroBitDisplay::updateAnimateImage()
{   
    if (scrollingImagePosition <= -scrollingImage.getWidth() && scrollingImageRendered)
    {
        animationMode = ANIMATION_MODE_NONE;  
        this->sendEvent(MICROBIT_DISPLAY_EVT_ANIMATION_COMPLETE);     
        return;
    }
    
    image.clear();     
    image.paste(scrollingImage, scrollingImagePosition, 0, 0);
    
    scrollingImageRendered = true;
    scrollingImagePosition += scrollingImageStride;
}

/**
  * Resets the current given animation.
  * @param delay the delay after which the animation is reset.
  */
void MicroBitDisplay::resetAnimation(uint16_t delay)
{
    //sanitise this value
    if(delay <= 0 )
        delay = MICROBIT_DEFAULT_SCROLL_SPEED;
        
    // Reset any ongoing animation.
    if (animationMode != ANIMATION_MODE_NONE)
    {
        animationMode = ANIMATION_MODE_NONE;
        this->sendEvent(MICROBIT_DISPLAY_EVT_ANIMATION_COMPLETE);
    }
    
    // Clear the display and setup the animation timers.
    this->image.clear();
    this->animationDelay = delay;
    this->animationTick = delay-1;
}

/**
  * Prints the given character to the display.
  *
  * @param c The character to display.
  * 
  * Example:
  * @code 
  * uBit.display.print('p');
  * @endcode
  */
void MicroBitDisplay::print(char c)
{
    image.print(c, 0, 0);
}

/**
  * Prints the given string to the display, one character at a time.
  * Uses the given delay between characters.
  * Returns immediately, and executes the animation asynchronously.
  *
  * @param s The string to display.
  * @param delay The time to delay between characters, in timer ticks.
  * 
  * Example:
  * @code 
  * uBit.display.printStringAsync("abc123",400);
  * @endcode
  */
void MicroBitDisplay::printStringAsync(ManagedString s, uint16_t delay)
{
    //sanitise this value
    if(delay <= 0 )
        delay = MICROBIT_DEFAULT_SCROLL_SPEED;
    
    this->resetAnimation(delay);
    
    this->printingChar = 0;
    this->printingText = s;
    
    animationMode = ANIMATION_MODE_PRINT_TEXT;
}

/**
  * Prints the given string to the display, one character at a time.
  * Uses the given delay between characters.
  * Blocks the calling thread until all the text has been displayed.
  *
  * @param s The string to display.
  * @param delay The time to delay between characters, in timer ticks.
  * 
  * Example:
  * @code 
  * uBit.display.printString("abc123",400);
  * @endcode
  */
void MicroBitDisplay::printString(ManagedString s, uint16_t delay)
{
    //sanitise this value
    if(delay <= 0 )
        delay = MICROBIT_DEFAULT_SCROLL_SPEED;
    
    // Start the effect.
    this->printStringAsync(s, delay);
    
    // Wait for completion.
    fiber_wait_for_event(MICROBIT_ID_DISPLAY, MICROBIT_DISPLAY_EVT_ANIMATION_COMPLETE);
}


/**
  * Scrolls the given string to the display, from right to left.
  * Uses the given delay between characters.
  * Returns immediately, and executes the animation asynchronously.
  *
  * @param s The string to display.
  * @param delay The time to delay between characters, in timer ticks.
  * 
  * Example:
  * @code 
  * uBit.display.scrollStringAsync("abc123",100);
  * @endcode
  */
void MicroBitDisplay::scrollStringAsync(ManagedString s, uint16_t delay)
{
    //sanitise this value
    if(delay <= 0 )
        delay = MICROBIT_DEFAULT_SCROLL_SPEED;
    
    this->resetAnimation(delay);
    
    this->scrollingPosition = width-1;
    this->scrollingChar = 0;
    this->scrollingText = s;
    
    animationMode = ANIMATION_MODE_SCROLL_TEXT;
}

/**
  * Scrolls the given string to the display, from right to left.
  * Uses the given delay between characters.
  * Blocks the calling thread until all the text has been displayed.
  *
  * @param s The string to display.
  * @param delay The time to delay between characters, in timer ticks.
  * 
  * Example:
  * @code 
  * uBit.display.scrollString("abc123",100);
  * @endcode
  */
void MicroBitDisplay::scrollString(ManagedString s, uint16_t delay)
{
    //sanitise this value
    if(delay <= 0 )
        delay = MICROBIT_DEFAULT_SCROLL_SPEED;
    
    // Start the effect.
    this->scrollStringAsync(s, delay);
    
    // Wait for completion.
    fiber_wait_for_event(MICROBIT_ID_DISPLAY, MICROBIT_DISPLAY_EVT_ANIMATION_COMPLETE);
}


/**
  * Scrolls the given image across the display, from right to left.
  * Returns immediately, and executes the animation asynchronously.
  * @param image The image to display.
  * @param delay The time to delay between each scroll update, in timer ticks. Has a default.
  * @param stride The number of pixels to move in each quantum. Has a default.
  * 
  * Example:
  * @code 
  * MicrobitImage i("1,1,1,1,1\n1,1,1,1,1\n"); 
  * uBit.display.scrollImageAsync(i,100,1);
  * @endcode
  */
void MicroBitDisplay::scrollImageAsync(MicroBitImage image, uint16_t delay, int8_t stride)
{
    // Assume right to left functionality, to align with scrollString()
    stride = -stride;
    
    //sanitise the delay value
    if(delay <= 0 )
        delay = MICROBIT_DEFAULT_SCROLL_SPEED;
            
    this->resetAnimation(delay);

    this->scrollingImagePosition = stride < 0 ? width : -image.getWidth();
    this->scrollingImageStride = stride;
    this->scrollingImage = image;
    this->scrollingImageRendered = false;
        
    animationMode = ANIMATION_MODE_SCROLL_IMAGE;
}

/**
  * Scrolls the given image across the display, from right to left.
  * Blocks the calling thread until all the text has been displayed.
  *
  * @param image The image to display.
  * @param delay The time to delay between each scroll update, in timer ticks. Has a default.
  * @param stride The number of pixels to move in each quantum. Has a default. 
  * 
  * Example:
  * @code 
  * MicrobitImage i("1,1,1,1,1\n1,1,1,1,1\n"); 
  * uBit.display.scrollImage(i,100,1);
  * @endcode
  */
void MicroBitDisplay::scrollImage(MicroBitImage image, uint16_t delay, int8_t stride)
{
    //sanitise the delay value
    if(delay <= 0 )
        delay = MICROBIT_DEFAULT_SCROLL_SPEED;
    
    // Start the effect.
    this->scrollImageAsync(image, delay, stride);
    
    // Wait for completion.
    fiber_wait_for_event(MICROBIT_ID_DISPLAY, MICROBIT_DISPLAY_EVT_ANIMATION_COMPLETE);
}

/**
  * "Animates" the current image across the display with a given stride, finishing on the last frame of the animation.
  * Returns immediately.
  *
  * @param image The image to display.
  * @param delay The time to delay between each animation update, in timer ticks. Has a default.
  * @param stride The number of pixels to move in each quantum. Has a default.
  * 
  * Example:
  * @code 
  * const int heart_w = 10;
  * const int heart_h = 5;
  * const uint8_t heart[] = { 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, };
  *
  * MicroBitImage i(heart_w,heart_h,heart);
  * uBit.display.animateImageAsync(i,100,5);
  * @endcode
  */
void MicroBitDisplay::animateImageAsync(MicroBitImage image, uint16_t delay, int8_t stride)
{
    // Assume right to left functionality, to align with scrollString()
    stride = -stride;
    
    //sanitise the delay value
    if(delay <= 0 )
        delay = MICROBIT_DEFAULT_SCROLL_SPEED;
            
    this->resetAnimation(delay);

    this->scrollingImagePosition = 0;//stride < 0 ? width : -image.getWidth();
    this->scrollingImageStride = stride;
    this->scrollingImage = image;
    this->scrollingImageRendered = false;
        
    animationMode = ANIMATION_MODE_ANIMATE_IMAGE;
}

/**
  * "Animates" the current image across the display with a given stride, finishing on the last frame of the animation.
  * Blocks the calling thread until the animation is complete.
  * 
  * @param image The image to display.
  * @param delay The time to delay between each animation update, in timer ticks. Has a default.
  * @param stride The number of pixels to move in each quantum. Has a default.
  * 
  * Example:
  * @code 
  * const int heart_w = 10;
  * const int heart_h = 5;
  * const uint8_t heart[] = { 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, };
  *
  * MicroBitImage i(heart_w,heart_h,heart);
  * uBit.display.animateImage(i,100,5);
  * @endcode
  */
void MicroBitDisplay::animateImage(MicroBitImage image, uint16_t delay, int8_t stride)
{
    //sanitise the delay value
    if(delay <= 0 )
        delay = MICROBIT_DEFAULT_SCROLL_SPEED;
    
    // Start the effect.
    this->animateImageAsync(image, delay, stride);
    
    // Wait for completion.
    fiber_wait_for_event(MICROBIT_ID_DISPLAY, MICROBIT_DISPLAY_EVT_ANIMATION_COMPLETE);
}


/**
  * Sets the display brightness to the specified level.
  * @param b The brightness to set the brightness to, in the range 0..255.
  * 
  * Example:
  * @code 
  * uBit.display.setBrightness(255); //max brightness
  * @endcode
  */  
void MicroBitDisplay::setBrightness(int b)
{
    //sanitise the brightness level
    if(b < 0)
        b = 0;
        
    if(b > 255)
        b = 255;
    
    float level = (float)b / float(MICROBIT_DISPLAY_MAX_BRIGHTNESS);
    
    this->brightness = b;
    this->rowDrive->write(level);
}

/**
  * Fetches the current brightness of this display.
  * @return the brightness of this display, in the range 0..255.
  * 
  * Example:
  * @code 
  * uBit.display.getBrightness(); //the current brightness
  * @endcode
  */  
int MicroBitDisplay::getBrightness()
{
    return this->brightness;
}

/**
  * Rotates the display to the given position.
  * Axis aligned values only.
  *
  * Example:
  * @code 
  * uBit.display.rotateTo(MICROBIT_DISPLAY_ROTATION_180); //rotates 180 degrees from original orientation
  * @endcode
  */   
void MicroBitDisplay::rotateTo(uint8_t position)
{
    //perform a switch on position to restrict range to distinct values
    switch(position){
        case MICROBIT_DISPLAY_ROTATION_0:
            this->rotation = MICROBIT_DISPLAY_ROTATION_0;
            break;   
        case MICROBIT_DISPLAY_ROTATION_90:
            this->rotation = MICROBIT_DISPLAY_ROTATION_90;
            break;
        case MICROBIT_DISPLAY_ROTATION_180:
            this->rotation = MICROBIT_DISPLAY_ROTATION_180;
            break;
        case MICROBIT_DISPLAY_ROTATION_270:
            this->rotation = MICROBIT_DISPLAY_ROTATION_270;
            break;
    }
}

/**
  * Enables the display, should only be called if the display is disabled.
  *
  * Example:
  * @code 
  * uBit.display.enable(); //reenables the display mechanics
  * @endcode
  */
void MicroBitDisplay::enable()
{
    if(!(uBit.flags & MICROBIT_FLAG_DISPLAY_RUNNING))
    {
        new(&columnPins) BusOut(MICROBIT_DISPLAY_COLUMN_PINS);  //bring columnPins back up
        columnPins.write(0xFFFF);                               //write 0xFFFF to reset all column pins 
        rowDrive = DynamicPwm::allocate(rowPins[0],PWM_PERSISTENCE_PERSISTENT); //bring rowDrive back up
        rowDrive->period_us(MICROBIT_DISPLAY_PWM_PERIOD);                          
        setBrightness(brightness);
        uBit.flags |= MICROBIT_FLAG_DISPLAY_RUNNING;            //set the display running flag
    }
}
    
/**
  * Disables the display, should only be called if the display is enabled.
  * Display must be disabled to avoid MUXing of edge connector pins.
  *
  * Example:
  * @code 
  * uBit.display.disable(); //disables the display
  * @endcode
  */
void MicroBitDisplay::disable()
{
    if(uBit.flags & MICROBIT_FLAG_DISPLAY_RUNNING)
    {
        uBit.flags &= ~MICROBIT_FLAG_DISPLAY_RUNNING;           //unset the display running flag
        columnPins.~BusOut();
        rowDrive->free();
    }   
}

/**
  * Clears the current image on the display.
  * Simplifies the process, you can also use uBit.display.image.clear
  *
  * Example:
  * @code 
  * uBit.display.clear(); //clears the display
  * @endcode
  */ 
void MicroBitDisplay::clear()
{
    image.clear();  
}

/**
  * Displays "=(" and an accompanying status code infinitely.
  * @param statusCode the appropriate status code - 0 means no code will be displayed. Status codes must be in the range 0-255.
  *
  * Example:
  * @code 
  * uBit.display.error(20);
  * @endcode
  */
void MicroBitDisplay::error(int statusCode)
{   
    __disable_irq(); //stop ALL interrupts

    if(statusCode < 0 || statusCode > 255)
        statusCode = 0;

    disable(); //relinquish PWMOut's control
    
    uint8_t strobeRow = 0;
    uint8_t strobeBitMsk = 0x20;
    
    //point to the font stored in Flash
    const unsigned char * fontLocation = MicroBitFont::defaultFont;
    
    //get individual digits of status code, and place it into a single array/
    const uint8_t* chars[MICROBIT_DISPLAY_ERROR_CHARS] = { panicFace, fontLocation+((((statusCode/100 % 10)+48)-MICROBIT_FONT_ASCII_START) * 5), fontLocation+((((statusCode/10 % 10)+48)-MICROBIT_FONT_ASCII_START) * 5), fontLocation+((((statusCode % 10)+48)-MICROBIT_FONT_ASCII_START) * 5)};
    
    //enter infinite loop.
    while(1)
    {
        //iterate through our chars :)
        for(int characterCount = 0; characterCount < MICROBIT_DISPLAY_ERROR_CHARS; characterCount++)
        {
            int outerCount = 0;
            
            //display the current character
            while( outerCount < 100000)
            {
                int coldata = 0;
        
                int i = 0;
        
                //if we have hit the row limit - reset both the bit mask and the row variable
                if(strobeRow == 3)
                {
                    strobeRow = 0; 
                    strobeBitMsk = 0x20;
                }    
        
                // Calculate the bitpattern to write.
                for (i = 0; i<MICROBIT_DISPLAY_COLUMN_COUNT; i++)
                {
                    
                    int bitMsk = 0x10 >> matrixMap[i][strobeRow].x; //chars are right aligned but read left to right
                    int y = matrixMap[i][strobeRow].y;
                         
                    if(chars[characterCount][y] & bitMsk)
                        coldata |= (1 << i);
                }
                
                nrf_gpio_port_write(NRF_GPIO_PORT_SELECT_PORT0, 0xF0); //clear port 0 4-7
                nrf_gpio_port_write(NRF_GPIO_PORT_SELECT_PORT1, strobeBitMsk | 0x1F); // clear port 1 8-12
                
                //write the new bit pattern
                nrf_gpio_port_write(NRF_GPIO_PORT_SELECT_PORT0, ~coldata<<4 & 0xF0); //set port 0 4-7
                nrf_gpio_port_write(NRF_GPIO_PORT_SELECT_PORT1, strobeBitMsk | (~coldata>>4 & 0x1F)); //set port 1 8-12
            
                //set i to an obscene number.
                i = 100000;
                
                //burn cycles
                while(i>0)
                    i--;
                
                //update the bit mask and row count
                strobeBitMsk <<= 1;    
                strobeRow++;
                outerCount++;
            }
        }
    }
}

/**
  * Updates the font property of this object with the new font.
  * @param font the new font that will be used to render characters..
  */
void MicroBitDisplay::setFont(MicroBitFont font)
{
    this->font = font;
}

/**
  * Retreives the font object used for rendering characters on the display.
  */
MicroBitFont MicroBitDisplay::getFont()
{
    return this->font;
}