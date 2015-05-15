#include "inc/MicroBitTest.h"

#ifdef MAIN_HELLOWORLD_TEST

#include "MicroBit.h"

#ifdef MICROBIT_DBG
Serial pc(USBTX, USBRX);
#endif

MicroBit uBit;

char *defaultMessage = "HI HOWARD! WANT TO PLAY?";
char bleMessage[50];
int update = 0;

void
updateScroll(char* str, int len)
{
    len = min(len,50);
    memcpy(bleMessage, str, len);
    bleMessage[len]=0;
    update = 1;
}


void dfuCallbackFn()
{
}

int main()
{    
    //scheduler_init();
    char msg[50];
    
    strcpy(msg, defaultMessage);
     
    while(1)
    {
        if (update)
        {
            update = 0;
            strcpy(msg, bleMessage);
        }
        
        uBit.display->scrollString(msg);
    
        for (int i=0; i<150; i++)
        {
            wait(0.1);
            
            if (uBit.ble)
                uBit.ble->waitForEvent();
            
            if (update)
                break;
        }
    }
}

#endif