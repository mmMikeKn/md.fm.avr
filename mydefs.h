
#ifndef __MYDEFS_H
#define __MYDEFS_H

#define F_CPU 13225600UL
#define TRUE 1
#define FALSE 0

#define LED_INIT { DDRD = _BV(PD0); } // PD0 - out
#define LED_OFF { PORTD |= _BV(PD0); }
#define LED_ON { PORTD &= ~_BV(PD0); }

#endif //__MYDEFS_H
