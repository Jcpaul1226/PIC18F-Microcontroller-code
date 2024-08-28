#include <stdio.h>
#include <stdlib.h>
#include <xc.h>
#include <math.h>
#include <p18f4620.h>
#include <usart.h>
#include <string.h>

#include "I2C.h"
#include "I2C_Support.h"
#include "Interrupt.h"
#include "Fan_Support.h"
#include "Main.h"
#include "ST7735_TFT.h"

#pragma config OSC = INTIO67
#pragma config BOREN =OFF
#pragma config WDT=OFF
#pragma config LVP=OFF
#pragma config CCP2MX = PORTBE

void Initialize_Screen();
char second = 0x00;
char minute = 0x00;
char hour = 0x00;
char dow = 0x00;
char day = 0x00;
char month = 0x00;
char year = 0x00;

char found;
char tempSecond = 0xff; 
signed int DS1621_tempC, DS1621_tempF;
char setup_second, setup_minute, setup_hour, setup_day, setup_month, setup_year;
char alarm_second, alarm_minute, alarm_hour, alarm_date;
char setup_alarm_second, setup_alarm_minute, setup_alarm_hour;
unsigned char setup_fan_temp = 75;
unsigned char Nec_state = 0;

short nec_ok = 0;
char Nec_code1;
char FAN;
char duty_cycle;
int rps;
int rpm;
int ALARMEN;                                                                    // Variable prototyping

char buffer[33]     = " ECE3301L Sp'21 L12\0";
char *nbr;
char *txt;
char tempC[]        = "+25";
char tempF[]        = "+77";
char time[]         = "00:00:00";
char date[]         = "00/00/00";
char alarm_time[]   = "00:00:00";
char Alarm_SW_Txt[] = "OFF";
char Fan_SW_Txt[]   = "OFF";                                                    // text storage for Heater Mode

char array1[21]={0xa2, 0x62, 0xe2, 0x22, 0x02, 0xc2, 0xe0, 0xa8, 0x90, 0x68, 0x98, 0xb0, 0x30, 0x18, 0x7a, 0x10, 0x38, 0x5a, 0x42, 0x4a, 0x52};
                                                                                // array of button locations
    
char DC_Txt[]       = "000";                                                    // text storage for Duty Cycle value
char RTC_ALARM_Txt[]= "0";                                                      //
char RPM_Txt[]      = "0000";                                                   // text storage for RPM

char setup_time[]       = "00:00:00";
char setup_date[]       = "01/01/00";
char setup_alarm_time[] = "00:00:00"; 
char setup_fan_text[]   = "075F";

void putch (char c)
{   
    while (!TRMT);       
    TXREG = c;
}

void init_UART()
{
    OpenUSART (USART_TX_INT_OFF & USART_RX_INT_OFF & USART_ASYNCH_MODE & USART_EIGHT_BIT & USART_CONT_RX & USART_BRGH_HIGH, 25);
    OSCCON = 0x70;
}

void Do_Init()                                                                  // Initialize the ports 
{ 
    init_UART();                                                                // Initialize the uart
    OSCCON=0x70;                                                                // Set oscillator to 8 MHz 
    
    ADCON1=0x0F;                                                                // Initializes ADCON1
    TRISA = 0x00;                                                               // Sets Port A as all output
    TRISB = 0x01;                                                               // Sets Port B as all output except lsb which is input
    TRISC = 0x01;                                                               // Sets Port C as all output except lsb which is input
    TRISD = 0x30;                                                               // Sets Port D's bit 4 and 5 as input and the rest as output
    TRISE = 0x00;                                                               // Sets Port E as all output

    RBPU=0;
    TMR3L = 0x00;                   
    T3CON = 0x03;                                                               // starts timer 3 at 0
    I2C_Init(100000); 

    DS1621_Init();
    init_INTERRUPT();                                                           // initiates Interrupt
    FAN = 0;                                                                    // initiates variable FAN
}

void main() 
{
    Do_Init();                                                                  // Initialization  
    Initialize_Screen();                                                        // Initializes TFT panel
   
    FAN_EN = 1;                                                                 // turns on fan
    duty_cycle = 100;                                                           // sets duty cycle to 100
    do_update_pwm(duty_cycle);                                                  // changes fan speed according to duty cycle
    while (1)                                                                   // endless loop
    {
        if (nec_ok ==1)                                                         // checks if button is pressed
        {
            nec_ok = 0;                                                         //releases button

            printf ("NEC_Code = %x\r\n", Nec_code1);                            // prints Nec_Code of button pressed

            INTCONbits.INT0IE = 1;                                              // Enable external interrupt
            INTCON2bits.INTEDG0 = 0;                                            // Edge programming for INT0 falling edge

            found = 0xff;
            for (int j=0; j< 21; j++)                                           // for loop that finds which button is pressed
            {
                if (Nec_code1 == array1[j])                                     // compares array entry and Nec_code to determine which button is pressed
                {
                    found = j;
                    j = 21;
                }
            }
            
            if (found == 0xff) 
            {
                printf ("Cannot find button \r\n");
            }
            else
            {
                Do_Beep();                                                      // buzzes upon button press
                printf ("button = %d \r\n", found);                             // prints number of which button used
                if (found == 5)
                {
                    Toggle_Fan();                                               // toggles fans on/off if >|| button pressed
                }
                else if (found == 6)
                {
                    Decrease_Speed();                                           // decreases speed of fan if - button is pressed
                }
                else if (found == 7)
                {
                    Increase_Speed();                                           // increases speed of fan if + button is pressed
                }
                else if (found == 8)
                {
                    DS3231_Setup_Time();                                        // resets to preset time if EQ button is pressed
                }
            }            
        }
        DS3231_Read_Time();                                                     // reads time
        if(tempSecond != second)
        {
            tempSecond = second;
            DS1621_tempC = DS1621_Read_Temp();                                  // reads temperature from sensor
            DS1621_tempF = (DS1621_tempC * 9 / 5) + 32;                         // converts the read temperature to fahrenheit
            rpm = get_RPM();                                                    // calls get_RPM store rpm value of the fan
            Set_DC_RGB(duty_cycle);                                             // sets an RBG LED's color according to duty_cycle value
            Set_RPM_RGB(rpm);                                                   // sets other RGB LED's color according to rpm value
            printf ("%02x:%02x:%02x %02x/%02x/%02x",hour,minute,second,month,day,year);
            printf (" Temp = %d C = %d F ", DS1621_tempC, DS1621_tempF);
            printf ("RPM = %d  dc = %d\r\n", rpm, duty_cycle);
            Update_Screen();                                                    // prints time, date, celsius, fahrenheit, duty_cycle, and rpm on the TFT panel and TeraTerm
        }
    }
}