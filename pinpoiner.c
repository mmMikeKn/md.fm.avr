#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#define F_CPU 13225600UL
#include <util/delay.h>

#define TRUE 1
#define FALSE 0

#define LED_INIT { DDRD |= _BV(PD0); } // PD0 - out
#define LED_OFF { PORTD |= _BV(PD0); }
#define LED_ON { PORTD &= ~_BV(PD0); }
#define VIBR_INIT { DDRD |= _BV(PD5); }
#define VIBR_ON  { PORTD &= ~_BV(PD5); }
#define VIBR_OFF { PORTD |= _BV(PD5); }

#define COMPARE_LEVEL 12
#define COMPARE_LEVEL_OVER 40
#define MAX_COMPARE 20

#define AVER_BUF_SZ 128
#define AVER_SHIFT 7
// AVER_BUF_SZ /38Hz  = average period

volatile unsigned short _tachValue;
volatile unsigned char _isTachValueReady, _isSoundOn;

ISR(TIMER2_COMP_vect) {
 TCNT2 = 0;
 if(_isSoundOn) {
  if((PORTB & _BV(PB1)) != 0) { PORTB |= _BV(PB2); PORTB &= ~_BV(PB1);
  } else {  PORTB |= _BV(PB1); PORTB &= ~_BV(PB2); }
 }
}

ISR(TIMER0_OVF_vect) {
 _tachValue = TCNT1; 
 TCNT1 = 0; TCNT0 = 0;   
// debug
// if((PORTB & _BV(PB2)) == 0) PORTB |= _BV(PB2);
// else PORTB &= ~_BV(PB2);
 _isTachValueReady = TRUE;
}
                  

unsigned short _storedValue[AVER_BUF_SZ];
unsigned char _storedValuePtr = 0;

int main(void) {
 unsigned short comparisonFrValue = 0, comparisonCnt = 0, firstFrValue;
 unsigned short v;

// debug port
 DDRB |= _BV(PB2); PORTB |= _BV(PB2);

//--------- configure block --------------
 LED_INIT
 VIBR_INIT

// configure Timer0 as counter of input fr.
// (XCK/T0) PD4 - External clock source on T0 pin. Clock on rising edge.
 DDRD &= ~_BV(PD4);// PORTD |= _BV(PD4); // pull up T0 source
 TCCR0 |= _BV(CS00) | _BV(CS01) | _BV(CS02);  
 TIMSK |= _BV(TOIE0);  // Enable timer 0 overflow interrupt  

// configure Timer1 as counter (Normal)
 TCCR1A=(0<<WGM11)|(0<<WGM10); 
 TCCR1B=(0<<WGM13)|(0<<WGM12)|(0<<CS12)|(0<<CS11)|(1<<CS10); 
 TCNT1 = 0;

//configure Timer2 as counter (Normal)
// DDRB |= _BV(PB1) | _BV(PB2);
 OCR2 = 100;
 _isSoundOn = FALSE;
 TCCR2 |= _BV(CS22) | _BV(CS21) | _BV(CS20);
// TIMSK |= _BV(OCIE2);  // Enable timer 0 overflow interrupt  
 

 _delay_ms(200);
 VIBR_OFF
 _delay_ms(200);

 sei();

//---------- calibrate loop ------------
 while(TRUE) {
  LED_ON
  if(_isTachValueReady) {
   LED_OFF
   v = _tachValue; _isTachValueReady = FALSE;
   if(abs(comparisonFrValue-v) <= COMPARE_LEVEL) {
    comparisonCnt++;
    if(comparisonCnt > MAX_COMPARE) break;
   } else {
    comparisonCnt = 0;
    comparisonFrValue = v;
   }
  }
 }
 LED_OFF
 _storedValuePtr = 0; firstFrValue = comparisonFrValue;
 for(v = 0; v < AVER_BUF_SZ; v++) _storedValue[v] = comparisonFrValue;

 VIBR_ON
 _delay_ms(200);
 VIBR_OFF
 _delay_ms(200);

//-------- main cycle --------------
 while(TRUE) {
  if(_isTachValueReady) {
   LED_OFF
   VIBR_OFF
 //  _isSoundOn = FALSE;
   v = _tachValue; _isTachValueReady = FALSE;
   
   if(comparisonFrValue < v && (v-comparisonFrValue) > COMPARE_LEVEL) {
//    _isSoundOn = TRUE; OCR2 = 10+v-comparisonFrValue;
    LED_ON
    VIBR_ON
   } else {
    if(comparisonFrValue >= v && (comparisonFrValue-v) > COMPARE_LEVEL) {
//     _isSoundOn = TRUE; OCR2 = 50+v-comparisonFrValue;
     VIBR_ON
	}
   }   
     // temperature drift correction
   if(abs(firstFrValue - v) < COMPARE_LEVEL_OVER) {
    unsigned long s = 0;
    if(_storedValuePtr >= AVER_BUF_SZ) _storedValuePtr = 0;
    _storedValue[_storedValuePtr++] = v;
    for(v = 0; v < AVER_BUF_SZ; v++) s += _storedValue[v];
    comparisonFrValue = s >> AVER_SHIFT;
   }
  }
 }
}
