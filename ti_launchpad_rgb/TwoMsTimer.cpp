/*
 TwoMsTimer.h - Leveraging timer with 2ms resolution
 
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
*/
 
#include "Energia.h"
#include "TwoMsTimer.h"

unsigned long TwoMsTimer::msecs;
void (*TwoMsTimer::func)();
volatile unsigned long TwoMsTimer::count;
volatile char TwoMsTimer::overflowing;

// lifted from wiring_analog.c so that the do not step on each other
// there should probably be a processor check here but that also exists in MsTimer.h
uint16_t analog_period2 = F_CPU/490, analog_div2 = 0, analog_res2=255; // devide clock with 0, 2, 4, 8
#define PWM_PERIOD analog_period2 // F_CPU/490
#define PWM_DUTY(x) ( (unsigned long)x*PWM_PERIOD / (unsigned long)analog_res2 )

// wait on all register changes until they call start.
void TwoMsTimer::set(unsigned long ms, void (*f)()) {
  if (ms == 0)
    msecs = 1;
  else
    msecs = ms;		
  func = f;
}

// configure teh registers the same way as they are for analogWrite()
void TwoMsTimer::start() {
  count = 0;
  overflowing = 0;
  // identical to the wiring_analog.c pwm setup so is compatible
  TA1CCR0 = PWM_PERIOD;           // PWM Period
  TA1CTL = TASSEL_2 + MC_1+ analog_div2;       // SMCLK, up mode
  TA1CCTL0 |= CCIE;                             // CCR0 interrupt enabled
}

// turn off the interrupts but don't turn of the timer because may
// be in use for pwm.
void TwoMsTimer::stop() {
  // disable interrupt
  TA1CCTL0 &= ~CCIE;
}

// called by the ISR every time we get an interrupt
void TwoMsTimer::_ticHandler() {
  // It is a 500hz interrupt so each interrupt is 2msec
  count += 2;
  if (count >= msecs && !overflowing) {
    overflowing = 1;
    // subtract ms to catch missed overflows. set to 0 if you don't want this.
    // do this before running supplied function so don't take into account handler time
    count = count - msecs; 
    // call the program supplied function
    (*func)();
    overflowing = 0;
  }
}

// Timer interrupt service routine
__attribute__((interrupt(TIMER1_A0_VECTOR)))
void Timer_A_int(void)
{
  TwoMsTimer::_ticHandler();
}


