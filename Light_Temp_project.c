#include <stdio.h>
#include <stdlib.h>
#include <xc.h>
#include <math.h>
#include <p18f4620.h>
#include <usart.h>

#pragma config OSC = INTIO67
#pragma config WDT = OFF
#pragma config LVP = OFF
#pragma config BOREN = OFF

#define delay 5
#define D1_RED PORTBbits.RB0
#define D1_GREEN PORTBbits.RB1
#define D1_BLUE PORTBbits.RB2
#define D2_RED PORTBbits.RB3
#define D2_GREEN PORTBbits.RB4
#define D2_BLUE PORTBbits.RB5
#define D3_RED PORTAbits.RA4
#define D3_GREEN PORTAbits.RA5

void SET_D1_WHITE()
{
    D1_RED = 1;
    D1_GREEN = 1;
    D1_BLUE = 1;
}  

char array[10] =
{0x40, 0x79, 0x24, 0x30, 0x19, 0x12, 0x02, 0x78, 0x00, 0x10};

void putch (char c)
{
    while (!TRMT);
    TXREG = c;
}

void Init_UART()
{
 OpenUSART (USART_TX_INT_OFF & USART_RX_INT_OFF &
USART_ASYNCH_MODE & USART_EIGHT_BIT & USART_CONT_RX &
USART_BRGH_HIGH, 25);
 OSCCON = 0x60;
} 

void Delay_One_Sec()                                             // To avoid data/LEDs moving too fast to see outputs
{
    for (int I = 0; I < 17000; I++);
}

void Init_ADC(void)
{
    ADCON1 = 0x1B;                                               // Sets AN0, AN1, and AN3 as analog
    ADCON2 = 0xA9;
}

unsigned int Get_Full_ADC(void)     //measures how many steps it takes to get to the unknown voltage
{
    int result;
    ADCON0bits.GO=1;
    while(ADCON0bits.DONE==1);
    result = (ADRESH * 0x100) + ADRESL;
    return result;
}

void Select_ADC_Channel( char channel)
{
    ADCON0 = channel * 4 + 1;                                   // Allows select b/w temperature reading (AN0) and light sensor (AN1)
                                                                // by shifting the bit 0 (set as 1) left 2 spaces then
                                                                // adding 1 to bit 0 to ensure it reads AN1 if needed.
                                                                // If AN0 is needed, ADCON0 is cleared (0*4 = 0) and added by 1 to select AN0.
}

void Display_Lower_Digit (char digit)
{
    PORTD = array[digit];
}

void Display_Upper_Digit (char digit)
{
    PORTC = array[digit];
    PORTC = PORTC & 0x3f;
    if (array[digit] >> 6)
    {
        PORTE = 0x2;;
    }
    else PORTE = 0x0;
}

void SET_D2_RED()
{
    D2_RED = 1;
    D2_GREEN = 0;
    D2_BLUE = 0;
}

void SET_D2_GREEN()
{
    D2_RED = 0;
    D2_GREEN = 1;
    D2_BLUE = 0;
}

void SET_D2_BLUE()
{
    D2_RED = 0;
    D2_GREEN = 0;
    D2_BLUE = 1;
}

void SET_D2_WHITE()
{
    D2_RED = 1;
    D2_GREEN = 1;
    D2_BLUE = 1;
}

void SET_D2_OFF()
{
    D2_RED = 0;
    D2_GREEN = 0;
    D2_BLUE = 0;
}

void SET_D3_RED()
{
    D3_RED = 1;
    D3_GREEN = 0;
}

void SET_D3_GREEN()
{
    D3_RED = 0;
    D3_GREEN = 1;
}

void SET_D3_YELLOW()
{
    D3_RED = 1;
    D3_GREEN = 1;
}

void DO_DISPLAY_D1(int color1) //color/10; 
{
    if (color1 >= 70){
        SET_D1_WHITE();
    }
    else{ 
        char tens = color1 / 10;
        char binaryNum[32];
        char i = 0;
        while (tens > 0)
        {
            binaryNum[i] = tens % 2;
            tens = tens / 2;
            i++;
        }
        D1_RED = binaryNum[0];
        D1_GREEN = binaryNum[1];
        D1_BLUE = binaryNum[2];  
    }
}
void DO_DISPLAY_D2(int color2)
{
    if (color2 >= 46 && color2 < 56){
        SET_D2_RED();  
    } else if(color2 >= 56 && color2 < 66){
        SET_D2_GREEN();
    } else if(color2 >= 66 && color2 < 76){
        SET_D2_BLUE();
    } else if (color2 >= 76){
        SET_D2_WHITE();
    } else SET_D2_OFF();
}

void DO_DISPLAY_D3(float color3)
{
    float volts = color3 / 1000;
    if (volts < 2.5){
        SET_D3_RED();
    } else if (volts >= 2.5 && volts < 3.4){
        SET_D3_GREEN();
    } else if (volts >= 3.4){
        SET_D3_YELLOW();
    }
}

void main(void)
{                                       
    Init_ADC();
    Init_UART();
    
    TRISA = 0x0F;                                                // Initialize TRISA with 0x0F to read as input
    TRISB = 0x00;                                                // Initialize TRISB with 0x00 to read as output
    TRISC = 0x00;                                                // Initialize TRISC with 0x00 to read as output              
    TRISD = 0x00;                                                // Initialize TRISD with 0x00 to read as output
    TRISE = 0x00;                                                // Initialize TRISE with 0x00 to read as output
    
    while (1){
        
        Select_ADC_Channel (0);
        int num_step = Get_Full_ADC();
        float voltage_mv = num_step * 4.0;
        float temperature_C = (1035.0 - voltage_mv) / 5.50;
        float temperature_F = 1.80 * temperature_C + 32.0;
        int tempF = (int) temperature_F;
        char U = tempF / 10;
        char L = tempF % 10;
        Display_Upper_Digit(U);
        Display_Lower_Digit(L);
        printf ("Temperature = %d F \r\n\n", tempF);
        DO_DISPLAY_D1 (tempF);
        DO_DISPLAY_D2 (tempF);
        
        Select_ADC_Channel (2);
        int num_step = Get_Full_ADC();
        float voltage_mv = num_step * 4.0;
        int volt_mv = (int) voltage_mv;
        printf ("Light Voltage = %d mV \r\n", volt_mv);
        DO_DISPLAY_D3 (voltage_mv);

        
        Delay_One_Sec();
        }
    }
    
//    while (1) {
//        for (int i = 0; i < 2; i++) 
//        {
//            Display_Lower_Digit(i);                             // Output lower digit of the 7-seg. display
//            Display_Upper_Digit(i);                             // Output upper digit of the 7-seg. display
//            Delay_One_Sec();
//        }
//        PORTC = 0xFF;
//        PORTD = 0xFF;
//        
//        for (int i = 0; i < 8; i++)
//        {
//            PORTB = i;
//            Delay_One_Sec();
//        }
//        PORTB = 0x00;
//        
//        for (int i = 0; i < 8; i++)
//        {
//            PORTB = i<<3;
//            Delay_One_Sec();
//        }
//        PORTB = 0x00;
//        
//        for (int i = 0; i < 4; i++)
//        {
//            PORTA = i << 4;
//            Delay_One_Sec();
//        }
//        PORTA = 0x00;
//    }
//}

//void SET_D1_RED()
//{
//    D1_RED = 1;
//    D1_GREEN = 0;
//    D1_BLUE = 0;
//}
//
//void SET_D1_GREEN()
//{
//    D1_RED = 0;
//    D1_GREEN = 1;
//    D1_BLUE = 0;
//}   
//
//void SET_D1_YELLOW()
//{
//    D1_RED = 1;
//    D1_GREEN = 1;
//    D1_BLUE = 0;
//}  
//
//void SET_D1_BLUE()
//{
//    D1_RED = 0;
//    D1_GREEN = 0;
//    D1_BLUE = 1;
//}   
//
//void SET_D1_PURPLE()
//{
//    D1_RED = 1;
//    D1_GREEN = 0;
//    D1_BLUE = 1;
//} 
//
//void SET_D1_CYAN()
//{
//    D1_RED = 0;
//    D1_GREEN = 1;
//    D1_BLUE = 1;
//}   
//

//
//void SET_D1_OFF()
//{
//    D1_RED = 0;
//    D1_GREEN = 0;
//    D1_BLUE = 0;
//}   
