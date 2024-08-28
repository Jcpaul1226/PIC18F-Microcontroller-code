#include <stdio.h>
#include <stdlib.h>
#include <xc.h>
#include <math.h>
#include <p18f4620.h>
#include <usart.h>
#include <string.h>

#pragma config OSC = INTIO67
#pragma config WDT = OFF
#pragma config LVP = OFF
#pragma config BOREN = OFF
#pragma config CCP2MX = PORTBE

#include "ST7735.h"
#define _XTAL_FREQ  8000000                                                     // Set operation for 8 Mhz

void Init_ADC(void);
void TIMER1_isr(void);
void INT0_isr(void);
void Initialize_Screen();
void Activate_Buzzer(void);
void Deactivate_Buzzer(void);
void Wait_Half_Second(void);
void Wait_One_Second_With_Beep(void);                                           //prototyping area

unsigned char Nec_state = 0;
unsigned char i,bit_count;
short nec_ok = 0;
unsigned long long Nec_code;
char Nec_code1;
unsigned int Time_Elapsed;

// colors
#define RD               ST7735_RED
#define BU               ST7735_BLUE
#define GR               ST7735_GREEN
#define MA               ST7735_MAGENTA
#define BK               ST7735_BLACK
#define WT               ST7735_WHITE

#define Circle_Size     20                                                      // Size of Circle for Light
#define Circle_X        60                                                      // Location of Circle
#define Circle_Y        80                                                      // Location of Circle
#define Text_X          52
#define Text_Y          77
#define TS_1            1                                                       // Size of Normal Text
#define TS_2            2                                                       // Size of Big Text


char buffer[31];                                                                // general buffer for display purpose
char *nbr;                                                                      // general pointer used for buffer
char *txt;

char array1[21]={0xa2, 0x62, 0xe2, 0x22, 0x02, 0xc2, 0xe0, 0xa8, 0x90, 0x68, 0x98, 0xb0, 0x30, 0x18, 0x7a, 0x10, 0x38, 0x5a, 0x42, 0x4a, 0x52};
char txt1[21][4] ={"CH-\0", "CH \0", "CH+\0", "|<<\0", ">>|\0", ">||\0", "VL-\0", "VL+\0", "EQ \0", " 0 \0", "100\0", "200\0", " 1 \0", " 2 \0", " 3 \0", " 4 \0", " 5 \0", " 6 \0", " 7 \0", " 8 \0", " 9 \0", };
int color[21]={RD, RD, RD, BU, BU, GR, MA, MA, MA, BK, BK, BK, BK, BK, BK, BK, BK, BK, BK, BK, BK};
char ledA[21]={0x00, 0x00, 0x01, 0x00, 0x00, 0x02, 0x00, 0x00, 0x05, 0x00, 0x00, 0x07, 0x00, 0x00, 0x07, 0x00, 0x00, 0x07, 0x00, 0x00, 0x07};
char ledD[21]={0x00, 0x01, 0x00, 0x00, 0x04, 0x00, 0x00, 0x05, 0x00, 0x00, 0x07, 0x00, 0x00, 0x07, 0x00, 0x00, 0x07, 0x00, 0x00, 0x07, 0x00};
char ledE[21]={0x01, 0x00, 0x00, 0x04, 0x00, 0x00, 0x05, 0x00, 0x00, 0x07, 0x00, 0x00, 0x07, 0x00, 0x00, 0x07, 0x00, 0x00, 0x07, 0x00, 0x00};

                                                                                //array 1 for button "locations"
                                                                                //array txt1 for the labels of each button
                                                                                //array color for the colors of each button displayed on the TFT panel
                                                                                //arrays ledA, ledD, and ledE stores the correct color order or off to match the order of the buttons

void putch (char c)
{
    while (!TRMT);
    TXREG = c;
}

void Init_ADC()
{
    ADCON0 = 0x01;
    ADCON2 = 0xA9;
}

void init_UART()
{
    OpenUSART (USART_TX_INT_OFF & USART_RX_INT_OFF &
    USART_ASYNCH_MODE & USART_EIGHT_BIT & USART_CONT_RX &
    USART_BRGH_HIGH, 25);
    OSCCON = 0x60;
}

void interrupt high_priority chkisr()                                           //checks if button is pressed and goes into isr routine
{
    if (PIR1bits.TMR1IF == 1) TIMER1_isr();
    if (INTCONbits.INT0IF == 1) INT0_isr();
}

void TIMER1_isr(void)
{
    Nec_state = 0;                                                              // Reset decoding process
    INTCON2bits.INTEDG0 = 0;                                                    // Edge programming for INT0 falling edge
    T1CONbits.TMR1ON = 0;                                                       // Disable T1 Timer
    PIR1bits.TMR1IF = 0;                                                        // Clear interrupt flag
}

void force_nec_state0()
{
    Nec_state=0;                                                                // Sets Nec_State to state 0 
    T1CONbits.TMR1ON = 0;                                                       // Disable T1 Timer
}

void INT0_isr(void)
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

void Activate_Buzzer()                                                          //activates continuous buzz
{
    PR2 = 0b11111001;
    T2CON = 0b00000101;
    CCPR2L = 0b01001010;
    CCP2CON = 0b00111100;
}

void Deactivate_Buzzer()                                                        //deactivates continuous buzz
{
    CCP2CON = 0X0;
    PORTBbits.RB3 = 0;
}

void Delay_One_Second()
{
    for (int I = 0; I < 17000; I++);                                            //waits one second without using timer
}

void Wait_One_Second_With_Beep()                                                // creates one second delay as well as sound buzzer
{
    Activate_Buzzer();                                                          //activates buzzer
    Delay_One_Second();                                                         // Wait for one second
    Deactivate_Buzzer();                                                        //deactivates buzzer
}

void main()
{
    Init_ADC();
    init_UART();
    OSCCON = 0x70;                                                              // 8 Mhz
    nRBPU = 0;                                                                  // Enable PORTB internal pull up resistor
    TRISA = 0x00;                                                               // PORTA as output
    TRISB = 0x01;                                                               // PORTB's lowest bit as input and the rest as output
    TRISC = 0x00;                                                               // PORTC as output
    TRISD = 0x00;                                                               // PORTD as output
    TRISE = 0x00;                                                               // PORTE as output
    ADCON1 = 0x0F;
    Initialize_Screen();
    INTCONbits.INT0IF = 0;                                                      // Clear external interrupt
    INTCON2bits.INTEDG0 = 0;                                                    // Edge programming for INT0 falling edge H to L
    INTCONbits.INT0IE = 1;                                                      // Enable external interrupt
    TMR1H = 0;                                                                  // Reset Timer1
    TMR1L = 0;                                                                  
    PIR1bits.TMR1IF = 0;                                                        // Clear timer 1 interrupt flag
    PIE1bits.TMR1IE = 1;                                                        // Enable Timer 1 interrupt
    INTCONbits.PEIE = 1;                                                        // Enable Peripheral interrupt
    INTCONbits.GIE = 1;                                                         // Enable global interrupts
    nec_ok = 0;                                                                 // Clear flag
    Nec_code = 0x0;                                                             // Clear code
   
    while(1)
    {
        if (nec_ok == 1)                                                        // checks interrupt flag
        {
            nec_ok = 0;                                                         // clears flag

            Nec_code1 = (char) ((Nec_code >> 8));
            printf ("NEC_Code = %08lx %x\r\n", Nec_code, Nec_code1);
            INTCONbits.INT0IE = 1;                                              // Enable external interrupt
            INTCON2bits.INTEDG0 = 0;                                            // Edge programming for INT0 falling edge
           
            char found = 0xff;
           
            for (int i=0; i<21; i++)                                            //for loop that finds the pressed button
            {
                if(Nec_code1 == array1[i])
                {
                    found = (char) i;
                    printf("found = %x \r\n i = %x \r\n", found, i);
                    i = 21;                                                     //force exits for loop
                }
            }
           
            if (found != 0xff)
            {
                fillCircle(Circle_X, Circle_Y, Circle_Size, color[found]);
                drawCircle(Circle_X, Circle_Y, Circle_Size, ST7735_WHITE);  
                drawtext(Text_X, Text_Y, txt1[found], ST7735_WHITE, ST7735_BLACK,TS_1);
               
                PORTA = ledA[found];                                            //turns PORT A's led correct color/off
                PORTD = ledD[found];                                            //turns PORT D's led correct color/off
                PORTE = ledE[found];                                            //turns PORT E's led correct color/off
               
                Wait_One_Second_With_Beep();
            }
        }
    }
}


void Initialize_Screen()
{
    LCD_Reset();
    TFT_GreenTab_Initialize();
    fillScreen(ST7735_BLACK);
 
    /* TOP HEADER FIELD */
    txt = buffer;
    strcpy(txt, "ECE3301L Spring 21-S4");  
    drawtext(2, 2, txt, ST7735_WHITE, ST7735_BLACK, TS_1);

    strcpy(txt, "LAB 10 ");  
    drawtext(50, 10, txt, ST7735_WHITE, ST7735_BLACK, TS_1);
}