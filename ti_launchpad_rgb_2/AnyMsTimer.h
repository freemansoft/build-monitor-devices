#ifndef AnyMsTimer_h
#define AnyMsTimer_h
#include "Energia.h"


#if defined(__MSP430G2452__) || defined(__MSP430G2553__) || defined(__MSP430G2231__) // LaunchPad specific
#include <inttypes.h>
#define _MSP430_CONFIG_
#undef _CORETEX_CONFIG_
#elif defined(TARGET_IS_BLIZZARD_RB1)|| defined(__LM4F120H5QR__) || defined(__TM4C123GH6PM__) // Stellaris and Tiva
#include <inttypes.h>
#define _CORTEX_CONFIG_
#undef _MSP430_CONFIG_
#else
#error "Board not supported"
#endif


namespace AnyMsTimer {
  extern unsigned long msecs;
  extern void (*func)();
  extern volatile unsigned long count;
  extern volatile char overflowing;
  extern volatile boolean running;

  void set(unsigned long ms, void (*f)());
  void start();
  void stop();
  void _ticHandler2MS();
  void _ticHandler(uint32_t ui32TimeMS);
}

#endif
