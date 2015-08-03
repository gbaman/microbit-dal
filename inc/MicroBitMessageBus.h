#ifndef MICROBIT_MESSAGE_BUS_H
#define MICROBIT_MESSAGE_BUS_H

#include "mbed.h"
#include "MicroBitEvent.h"

// Enumeration of core components.
#define MICROBIT_CONTROL_BUS_ID         0
#define MICROBIT_ID_ANY					0
#define MICROBIT_EVT_ANY				0

struct MicroBitListener
{
	uint16_t		id;				// The ID of the component that this listener is interested in. 
	uint16_t 		value;			// Value this listener is interested in receiving. 
	void* 			cb;				// Callback function associated with this listener. Either (*cb)(MicroBitEvent) or (*cb)(MicroBitEvent, void*) depending on whether cb_arg is NULL.
	void*			cb_arg;			// Argument to be passed to the caller. This is assumed to be a pointer, so passing in NULL means that the function doesn't take an argument.
	MicroBitEvent 	evt;
	
	MicroBitListener *next;

	/**
	  * Constructor. 
	  * Create a new Message Bus Listener.
	  * @param id The ID of the component you want to listen to.
	  * @param value The event ID you would like to listen to from that component
	  * @param handler A function pointer to call when the event is detected.
	  */
	MicroBitListener(uint16_t id, uint16_t value, void (*handler)(MicroBitEvent));
	
	/**
	  * Alternative constructor where we register a value to be passed to the
	  * callback. If arg == NULL, the function takes no extra arguemnt.
	  * Otherwise, the function is understood to take an extra argument.
	  */
	MicroBitListener(uint16_t id, uint16_t value, void *handler, void* arg);
};

struct MicroBitMessageBusCache
{
	int seq;
	MicroBitListener *ptr;
};
	

/**
  * Class definition for the MicroBitMessageBus.
  *
  * The MicroBitMessageBus is the common mechanism to deliver asynchronous events on the
  * MicroBit platform. It serves a number of purposes:
  *
  * 1) It provides an eventing abstraction that is independent of the underlying substrate.
  * 2) It provides a mechanism to decouple user code from trusted system code 
  *    i.e. the basis of a message passing nano kernel.
  * 3) It allows a common high level eventing abstraction across a range of hardware types.e.g. buttons, BLE...
  * 4) It provides a mechanims for extensibility - new devices added via I/O pins can have OO based
       drivers and communicate via the message bus with minima impact on user level languages.
  * 5) It allows for the possiblility of event / data aggregation, which in turn can save energy.
  * It has the following design principles:
  *
  * 1) Maintain a low RAM footprint where possible
  * 2) Make few assumptions about the underlying platform, but allow optimizations where possible.
  */
class MicroBitMessageBus
{
    public:

	/**
	  * Default constructor. 
	  * Anticipating only one MessageBus per device, as filtering is handled within the class.
	  */
    MicroBitMessageBus();    

	/**
	  * Send the given event to all regstered recipients.
	  *
	  * @param The event to send. This structure is assumed to be heap allocated, and will 
	  * be automatically freed once all recipients have been notified.
	  *
	  * THIS IS NOW WRAPPED BY THE MicroBitEvent CLASS FOR CONVENIENCE...
	  *
	  * Example:
      * @code 
	  * MicroBitEvent evt(id,MICROBIT_BUTTON_EVT_DOWN,ticks,false);
	  * evt.fire();
	  * //OR YOU CAN DO THIS...  
	  * MicroBitEvent evt(id,MICROBIT_BUTTON_EVT_DOWN);
      * @endcode
	  */
	void send(MicroBitEvent &evt);

	/**
	  * Send the given event to all regstered recipients, using a cached entry to minimize lookups.
	  * This is particularly useful for soptimizing ensors that frequently send to the same channel.
	  *
	  * @param evt The event to send. This structure is assumed to be heap allocated, and will 
	  * be automatically freed once all recipients have been notified.
	  * @param c Cache entry to reduce lookups for commonly used channels.
	  */
	void send(MicroBitEvent &evt, MicroBitMessageBusCache *c);

	/**
	  * Register a listener function.
	  * 
	  * @param id The source of messages to listen for. Events sent from any other IDs will be filtered. 
	  * Use MICROBIT_ID_ANY to receive events from all components.
	  *
	  * @param value The value of messages to listen for. Events with any other values will be filtered. 
	  * Use MICROBIT_EVT_ANY to receive events of any value.
	  *
	  * @param hander The function to call when an event is received.
	  * 
      * Example:
      * @code 
      * void onButtonBClick()
      * {
      * 	//do something
      * }
      * uBit.MessageBus.listen(MICROBIT_ID_BUTTON_B, MICROBIT_BUTTON_EVT_CLICK, onButtonBClick); // call function when ever a click event is detected.
      * @endcode
	  */
	void listen(int id, int value, void (*handler)(MicroBitEvent));
	
	/**
	  * Register a listener function.
	  * 
	  * Same as above, except the listener function is passed an extra argument in addition to the
	  * MicroBitEvent, when called.
	  */
	void listen(int id, int value, void (*handler)(MicroBitEvent, void*), void* arg);

	private:
	
	MicroBitListener *listeners;		// Chain of active listeners.
	int seq;							// Sequence number. Used to invalidate cache entries.
	void listen(int id, int value, void* handler, void* arg);
};

#endif


