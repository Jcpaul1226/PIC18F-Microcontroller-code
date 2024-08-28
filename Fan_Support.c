#include <p18f4620.h>
#include "Main.h"
#include "Fan_Support.h"
#include "stdio.h"

extern char FAN;
extern char duty_cycle;


int get_RPM()
{
    int RPS = TMR3L / 2;                                                        // read the count. Since there are 2 pulses per rev
                                                                                // then RPS = count /2
    TMR3L = 0;                                                                  // clear out the count
    return (RPS * 60);                                                          // return RPM = 60 * RPS
}

void Toggle_Fan()
{
    if (FAN == 1)
    {
        Turn_On_Fan();                                                          // calls Turn_On_Fan if FAN = 1
    }
    else
    {
        Turn_Off_Fan();                                                         // calls Turn_Off_Fan if FAN = 0
    }
}

void Turn_On_Fan()
{
    FAN = 1;                                                                    // sets variable FAN to 1
    do_update_pwm(duty_cycle);                                                  
    FAN_EN = 1;                                                                 // turns on fan to its current speed before it was turned off
    FANEN_LED = 1;                                                              // turns on fan led
}

void Turn_Off_Fan()
{
    FAN = 0;                                                                    // sets variable FAN to 0
    FAN_EN = 0;                                                                 // turns off fan
    FANEN_LED = 0;                                                              // turns off fan led
}

void Increase_Speed()
{
    if (duty_cycle == 100)                                                      // beeps twice if fan is at max speed
    {
        Do_Beep();
        Do_Beep();
    }
    else                                                                        // increases duty_cycle by 5 if fan is not at max speed
    {
        duty_cycle = duty_cycle + 5;
        do_update_pwm(duty_cycle);
    }
}

void Decrease_Speed()
{
    if (duty_cycle == 0)                                                        // beeps twice if fan is at min speed
    {
        Do_Beep();
        Do_Beep();
    }
    else                                                                        // decreases duty_cycle by 5 if fan is not at min speed
    {
        duty_cycle = duty_cycle - 5;
        do_update_pwm(duty_cycle);
    }
}

void do_update_pwm(char duty_cycle)                                             // changes the actual speed of the fan according to duty_cycle
{ 
    float dc_f;
    int dc_I;
    PR2 = 0b00000100 ;                                                          // set the frequency for 25 Khz
    T2CON = 0b00000111 ;                                                        //
    dc_f = ( 4.0 * duty_cycle / 20.0) ;                                         // calculate factor of duty cycle versus a 25 Khz
                                                                                // signal
    dc_I = (int) dc_f;                                                          // get the integer part
    if (dc_I > duty_cycle) dc_I++;                                              // round up function
    CCP1CON = ((dc_I & 0x03) << 4) | 0b00001100;
    CCPR1L = (dc_I) >> 2; 
}

void Set_DC_RGB(int duty_cycle)
{
    if (duty_cycle == 0)                                                        // turns LED 1 off if duty_cycle = 0
    {
        DC_RGB_RED = 0;
        DC_RGB_GREEN = 0;
    }
    else if (duty_cycle > 0 && duty_cycle < 50)                                 // turns LED 1 red if duty_cycle between 0 and 50
    {
        DC_RGB_RED = 1;
        DC_RGB_GREEN = 0;
    }
    else if (duty_cycle >= 50 && duty_cycle < 75)                               // turns LED 1 yellow if duty_cycle between 50 and 75
    {
        DC_RGB_RED = 1;
        DC_RGB_GREEN = 1;
    }
    else if (duty_cycle >= 75)                                                  // turns LED 1 green if duty_cycle above 75
    {
        DC_RGB_RED = 0;
        DC_RGB_GREEN = 1;
    }
}

void Set_RPM_RGB(int rpm)
{
    if (rpm == 0)                                                               // turns LED 2 off if rpm = 0
    {
        RPM_RGB_RED = 0;
        RPM_RGB_BLUE = 0;
    }
    else if (rpm > 0 && rpm < 1800)                                             // turns LED 2 red if rpm between 0 and 1800
    {
        RPM_RGB_RED = 1;
        RPM_RGB_BLUE = 0;
    }
    else if (rpm >= 1800 && rpm < 2700)                                         // turns LED 2 magenta if rpm between 1800 and 2700
    {
        RPM_RGB_RED = 1;
        RPM_RGB_BLUE = 1;
    }
    else if (rpm >= 2700)                                                       // turns LED 2 blue if rpm above 2700
    {
        RPM_RGB_RED = 0;
        RPM_RGB_BLUE = 1;
    }
}

void Do_Beep()                                                                  // beeps for one second then updates pwm to keep the fan speed
{
    Activate_Buzzer();
    Wait_One_Sec();
    Deactivate_Buzzer();
    do_update_pwm(duty_cycle);
}

void Wait_One_Sec()                                                             // waits for one second
{
    for(int I = 0; I < 17000; I++);
}

void Activate_Buzzer()                                                          // turns on continuous buzz
{
    PR2 = 0b11111001;
    T2CON = 0b00000101;
    CCPR2L = 0b01001010;
    CCP2CON = 0b00111100;
}

void Deactivate_Buzzer()                                                        // turns off continuous buzz
{
    CCP2CON = 0X0;
    PORTBbits.RB3 = 0;
}

