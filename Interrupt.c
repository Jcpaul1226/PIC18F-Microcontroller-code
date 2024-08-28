#include <xc.h>
#include <p18f4620.h>
#include "Interrupt.h"

extern unsigned char Nec_state;
extern short nec_ok;
extern unsigned long long Nec_code;
extern unsigned int Time_Elapsed;
extern unsigned char bit_count;

void TIMER1_isr(void);
void INT0_ISR(void);

void interrupt high_priority chkisr()
{
    if (PIR1bits.TMR1IF == 1) TIMER1_isr();
    if (INTCONbits.INT0IF == 1) INT0_ISR();
}

void TIMER1_isr(void)
{
    Nec_state = 0;                                                              // Reset decoding process
    INTCON2bits.INTEDG0 = 0;                                                    // Edge programming for INT0 falling edge
    T1CONbits.TMR1ON = 0;                                                       // Disable T1 Timer
    PIR1bits.TMR1IF = 0;
}

void force_nec_state0()
{
    Nec_state=0;                                                                // Sets Nec_State to state 0 
    T1CONbits.TMR1ON = 0;
}

void INT0_ISR() 
{    
    INTCONbits.INT0IF = 0;                                                      // Clear external interrupt
    if (Nec_state != 0)
    {
        Time_Elapsed = (TMR1H << 8) | TMR1L;                                    // Store Timer1 value
        TMR1H = 0;                                                              // Reset Timer1
        TMR1L = 0;
    }
   
    switch(Nec_state)
    {
        case 0 :
        {
                                                                                // Clear Timer 1
            TMR1H = 0;                                                          // Reset Timer1
            TMR1L = 0;                                                          
            PIR1bits.TMR1IF = 0;                                                
            T1CON= 0x90;                                                        // Program Timer1 mode with count = 1usec using System clock running at 8Mhz
            T1CONbits.TMR1ON = 1;                                               // Enable Timer 1
            bit_count = 0;                                                      // Force bit count (bit_count) to 0
            Nec_code = 0;                                                       // Set Nec_code = 0
            Nec_state = 1;                                                      // Set Nec_State to state 1
            INTCON2bits.INTEDG0 = 1;                                            // Change Edge interrupt of INT0 to Low to High            
            return;
        }
       
        case 1 :
        {
            if (Time_Elapsed > 8499 && Time_Elapsed < 9501)
            {
                Nec_state = 2;                                                  // Set Nec_State to state 2 if time elapsed is between 8500 and 9500 usec
            }
            else
            {
                force_nec_state0();                                             // forces Nec_State to state 0 if not within elapsed time requirement
            }
            INTCON2bits.INTEDG0 = 0;                                            // Change Edge interrupt of INT0 to High to Low
            return;
        }
       
        case 2 :
        {
            if (Time_Elapsed > 3999 && Time_Elapsed < 5001)
            {
                Nec_state = 3;                                                  // Set Nec_State to state 3 if time elapsed is between 4000 and 5000 usec
            }
            else
            {
                force_nec_state0();                                             // forces Nec_State to state 0 if not within elapsed time requirement
            }
            INTCON2bits.INTEDG0 = 1;                                            // Change Edge interrupt of INT0 to Low to High
            return;
        }
       
        case 3 :
        {
            if (Time_Elapsed > 399 && Time_Elapsed < 701)
            {
                Nec_state = 4;                                                  // Set Nec_State to state 4 if time elapsed is between 400 and 700 usec
            }
            else
            {
                force_nec_state0();                                             // forces Nec_State to state 0 if not within elapsed time requirement
            }
            INTCON2bits.INTEDG0 = 0;                                            // Change Edge interrupt of INT0 to High to Low
            return;
        }
       
        case 4 :
        {
            if (Time_Elapsed > 399 && Time_Elapsed < 1801)                      // Checks if Time_Elapsed is between 400 and 1800 usec
            {
                Nec_code = Nec_code << 1;                                       // left shifts Nec_code
                if (Time_Elapsed > 1000)                                        // Checks if Time_Elapsed is above 1000 usec
                {
                    Nec_code = Nec_code + 1;                                    // increments Nec_code
                }
                bit_count = bit_count + 1;                                      // increments bit_count
                if (bit_count > 31)                                             // checks of bit_count is over 31
                {
                    nec_ok = 1;                                                 // sets interrupt flag
                    INTCONbits.INT0IE = 0;                                      // clears external interrupt
                    Nec_state = 0;                                              // Set Nec_State to state 0
                }
                Nec_state = 3;                                                  // Set Nec_State to state 3
            }
            else
            {
                force_nec_state0();                                             // forces Nec_State to state 0 if not within elapsed time requirement
            }
            INTCON2bits.INTEDG0 = 1;                                            // Change Edge interrupt of INT0 to Low to High
            return;
        }
    }
} 



