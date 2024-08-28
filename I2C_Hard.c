#include <p18f4620.h>

#define ACK     1
#define NAK     0

#define _XTAL_FREQ      8000000

void I2C_Init(const unsigned long c)
{
  TRISC = 0x00;     
  SSPCON1 = 0b00101000;             // SSP Module as Master
  SSPCON2 = 0;
  SSPADD = (_XTAL_FREQ/(4*c))-1;    // Setting Clock Speed
  SSPSTAT = 0;
  TRISC3 = 1;                       // Setting as input as given in datasheet
  TRISC4 = 1;                       // Setting as input as given in datasheet
}

void I2C_Wait()
{
  while ((SSPSTAT & 0x04) || (SSPCON2 & 0x1F)); //Transmit is in progress
}

void I2C_Start()
{
  I2C_Wait();    
  SEN = 1;                          // Initiate start condition
}

void I2C_ReStart()
{
  I2C_Wait();
  RSEN = 1;                         // Initiate repeated start condition
}

void I2C_Stop()
{
  I2C_Wait();
  PEN = 1;                          // Initiate stop condition
}

void I2C_Write(unsigned char data_out)
{
  I2C_Wait();
  SSPBUF = data_out;                // Write data to SSPBUF
}

unsigned char I2C_Read(unsigned char a)
{
  unsigned char temp;
  I2C_Wait();
  RCEN = 1;
  I2C_Wait();
  temp = SSPBUF;                    // Read data from SSPBUF
  I2C_Wait();
  ACKDT = (a)?0:1;                  // Acknowledge bit
  ACKEN = 1;                        // Acknowledge sequence
  return temp;
}


char I2C_Write_Address_Read_One_Byte(char Device, char Address)
{
char Data_Ret;    
  I2C_Start();                      // Start I2C protocol
  I2C_Write((Device << 1) | 0);     // DS3231 address Write mode
  I2C_Write(Address);               // Send register address
  I2C_ReStart();                    // Restart I2C
  I2C_Write((Device << 1) | 1);     // Initialize data read
  Data_Ret = I2C_Read(NAK);         //
  I2C_Stop(); 
  return Data_Ret;
}

void I2C_Write_Address_Write_One_Byte(char Device, char Address, char Data_Out)
{
  I2C_Start();                      // Start I2C protocol
  I2C_Write((Device << 1) | 0);     // Device address Write mode
  I2C_Write(Address);               // Send register address
  I2C_Write(Data_Out);              // Initialize data read
  I2C_Stop(); 
}

void I2C_Write_Cmd_Only(char Device, char Cmd)
{
  I2C_Start();                      // Start I2C protocol
  I2C_Write((Device << 1) | 0);     // Device address Write mode
  I2C_Write(Cmd);                   // Send Command
  I2C_Stop(); 
}

void I2C_Write_Cmd_Write_Data(char Device, char Cmd, char Data_Out)
{
  I2C_Start();                      // Start I2C protocol
  I2C_Write((Device << 1) | 0);     // Device address Write mode
  I2C_Write(Cmd);                   // Send Command
  I2C_Write(Data_Out);              // Send Data
  I2C_Stop(); 
}

char I2C_Write_Cmd_Read_One_Byte(char Device, char Cmd)
{
char Data_Ret;    
  I2C_Start();                      // Start I2C protocol
  I2C_Write((Device << 1) | 0);     // Device address
  I2C_Write(Cmd);                   // Send register address
  I2C_ReStart();                    // Restart I2C
  I2C_Write((Device << 1) | 1);     // Initialize data read
  Data_Ret = I2C_Read(NAK);         //
  I2C_Stop(); 
  return Data_Ret;
}



