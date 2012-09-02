#ifndef TwoMsTimer_h
#define TwoMsTimer_h

#if defined(__MSP430G2452__) || defined(__MSP430G2553__) || defined(__MSP430G2231__) // LaunchPad specific
#include <inttypes.h>
#else
#error Board not supported
#endif

namespace TwoMsTimer {
	extern unsigned long msecs;
	extern void (*func)();
	extern volatile unsigned long count;
	extern volatile char overflowing;
	
	void set(unsigned long ms, void (*f)());
	void start();
	void stop();
	void _ticHandler();
}

#endif
