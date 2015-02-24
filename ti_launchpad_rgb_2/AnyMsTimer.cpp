/*
 AnyMsTimer.h - Leveraging timer with 2ms resolution
 
 History:
 31/Aug/12 - MSP430 hacked MSP430 version freemansoft.com
             API and overflow()are derived from Arduino MsTimer2
             by Javier Valencia <javiervalencia80@gmail.com>
 
 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.
 
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/*
 MSP430 Notes:
 This timer currently uses Timer1 available on chips like the MSP430G2553
 It uses CCR0 in interrupt mode with the same period as PWM so PWM is 
 still available for Timer1 which means pwm , analogWrite() is still available
 on the Timer1 associated pins.  We couild have hacked into the watchdog timer
 which is used for millis() but that would have been a core change.
 
 The MSP430/Energia Servo library uses Timer0 so this library should be able to 
 run concurrently with Servo.  
 
 It will not run at the same time as the interrupt driven software serial library
 because that uses both timer interrupt vectors
 
 Stellaris/Tiva Notes:
 This uses SysTick.  It registers an instance of this with registerSysTickCb delegating
 the work to the passed in handler.


 Note the differnce in behavior. 
   MSP430 starts and stops the timer.  More CPU constrained so better to give back clock cycles 
   Tiva / Stellaris just ignores the calls because can't unregister
*/
 
#include "AnyMsTimer.h"

unsigned long AnyMsTimer::msecs;
void (*AnyMsTimer::func)();
volatile unsigned long AnyMsTimer::count;
volatile char AnyMsTimer::overflowing;
volatile boolean AnyMsTimer::running;

/*
 MSP430: Wait on all register changes until they call start.
 Stellaris/Tiva start getting interrupts immediately som mask off with "running" flag
 Stellaris/tiva don't start and stop handler.  Just ignore it if not running
 ms is interrupt interval in ms
 f is the location of the function
*/
void AnyMsTimer::set(unsigned long ms, void (*f)()) {
  if (ms == 0)
    msecs = 1;
  else
    msecs = ms;		
  func = f;
  running = false;
#if defined _CORTEX_CONFIG_
  registerSysTickCb(AnyMsTimer::_ticHandler);
#endif
}


#if defined(_CORTEX_CONFIG_)

// configure the   registers the same way as they are for analogWrite()
void AnyMsTimer::start() {
  count = 0;
  overflowing = 0;
  running=true;
}

// turn off the interrupts but don't turn of the timer because may
// be in use for pwm.
void AnyMsTimer::stop() {
  //SysTick keeps running and there is no way to remove
  running=false;
}

// called by the SysTick ISR every time we get an interrupt
// copied from the 2MS handler for MSP430.  Operates independently from timer value
void AnyMsTimer::_ticHandler(uint32_t ui32TimeMS)
{
  if (running == true){
    //SysTick is 1msec
    count += 1;
    if (count >= msecs && !overflowing) {
      overflowing = 1;
      // subtract msecs to catch missed overflows. set to 0 if you don't want this.
      // do this before running supplied function so don't take into account handler time
      count = count - msecs; 
      // call the program supplied function
      (*func)();
      overflowing = 0;
    }
  }
}
#endif

#if defined(_MSP430_CONFIG_)
// lifted from wiring_analog.c so that the do not step on each other
// there should probably be a processor check here but that also exists in MsTimer.h
uint16_t analog_period2 = F_CPU/490, analog_div2 = 0, analog_res2=255; // devide clock with 0, 2, 4, 8
#define PWM_PERIOD analog_period2 // F_CPU/490
#define PWM_DUTY(x) ( (unsigned long)x*PWM_PERIOD / (unsigned long)analog_res2 )

// configure the   registers the same way as they are for analogWrite()
void AnyMsTimer::start() {
  count = 0;
  overflowing = 0;
  // identical to the wiring_analog.c pwm setup so is compatible
  TA1CCR0 = PWM_PERIOD;                  // PWM Period
  TA1CTL = TASSEL_2 + MC_1+ analog_div2; // SMCLK, up mode
  TA1CCTL0 |= CCIE;                      // CCR0 interrupt enabled
  running=true;
}

// turn off the interrupts but don't turn of the timer because may
// be in use for pwm.
void AnyMsTimer::stop() {
  // disable interrupt
  TA1CCTL0 &= ~CCIE;
  running=false;
}

// called by the ISR every time we get an interrupt
// operates indepndently from timer value
void AnyMsTimer::_ticHandler2MS()
{
  if (running == true){
    // It is a 500hz interrupt so each interrupt is 2msec
    count += 2;
    if (count >= msecs && !overflowing) {
      overflowing = 1;
      // subtract msecs to catch missed overflows. set to 0 if you don't want this.
      // do this before running supplied function so don't take into account handler time
      count = count - msecs; 
      // call the program supplied function
      (*func)();
      overflowing = 0;
    }
  }
}

// Timer interrupt service routine
__attribute__((interrupt(TIMER1_A0_VECTOR)))
void Timer_A_int(void)
{
  AnyMsTimer::_ticHandler2MS();
}
#endif

