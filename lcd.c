/******************************************************************************
Software License Agreement

The software supplied herewith by Microchip Technology Incorporated
(the "Company") for its PIC(R) Microcontroller is intended and
supplied to you, the Company's customer, for use solely and
exclusively on Microchip PICmicro Microcontroller products. The
software is owned by the Company and/or its supplier, and is
protected under applicable copyright laws. All rights are reserved.
Any use in violation of the foregoing restrictions may subject the
user to criminal sanctions under applicable laws, as well as to
civil liability for the breach of the terms and conditions of this
license.

THIS SOFTWARE IS PROVIDED IN AN "AS IS" CONDITION. NO WARRANTIES,
WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT,
IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 *******************************************************************************/

#include <xc.h>
//#include <lcd.h>
#include "lcd.h"
#include <stdint.h>

/* Private Definitions ***********************************************/
// Define a fast instruction execution time in terms of loop time
// typically > 4863us
#define LCD_F_INSTR         500

// Define a slow instruction execution time in terms of loop time
// typically > 3.35ms
#define LCD_S_INSTR         500

// Define the startup time for the LCD in terms of loop time
// typically > 60ms (double than 8 bit mode)
#define LCD_STARTUP         20000

#define LCD_MAX_COLUMN      16

#define LCD_COMMAND_CLEAR_SCREEN        0x01
#define LCD_COMMAND_RETURN_HOME         0x02
#define LCD_COMMAND_ENTER_DATA_MODE     0x06
#define LCD_COMMAND_CURSOR_OFF          0x0C
#define LCD_COMMAND_CURSOR_ON           0x0F
#define LCD_COMMAND_MOVE_CURSOR_LEFT    0x10
#define LCD_COMMAND_MOVE_CURSOR_RIGHT   0x14
#define LCD_COMMAND_SET_MODE_4_BIT      0x28
#define LCD_COMMAND_SET_MODE_8_BIT      0x38
#define LCD_COMMAND_ROW_0_HOME          0x80
#define LCD_COMMAND_ROW_1_HOME          0xC0
#define LCD_START_UP_COMMAND_1          0x33    
#define LCD_START_UP_COMMAND_2          0x32    

#define LCD_DATA_LAT                   LATB
#define LCD_DATA_TRIS                  TRISB
#define LCD_RSSignal_Set()        LATBbits.LATB3 = 1 //set Register Select bit
#define LCD_RSSignal_Clear()      LATBbits.LATB3 = 0 //clear Register Select bit
#define LCD_RWSignal_Set()        LATBbits.LATB6 = 1  //set Read/Write bit
#define LCD_RWSignal_Clear()      LATBbits.LATB6 = 0  //clear Read/Write bit
#define LCD_EnableSignal_Set()    LATBbits.LATB5 = 1  //set Enable bit
#define LCD_EnableSignal_Clear()  LATBbits.LATB5 = 0  //clear Enable bit
#define LCD_RSSignal_Input()      TRISBbits.TRISB3 = 1 //set Register Select bit
#define LCD_RSSignal_Output()     TRISBbits.TRISB3 = 0 //clear Register Select bit
#define LCD_RWSignal_Input()      TRISBbits.TRISB6 = 1  //set Read/Write bit
#define LCD_RWSignal_Output()     TRISBbits.TRISB6 = 0  //clear Read/Write bit
#define LCD_EnableSignal_Input()  TRISBbits.TRISB5 = 1  //set Enable bit
#define LCD_EnableSignal_Output() TRISBbits.TRISB5 = 0  //clear Enable bit

/* Private Functions *************************************************/
static void LCD_CarriageReturn ( void ) ;
static void LCD_ShiftCursorLeft ( void ) ;
static void LCD_ShiftCursorRight ( void ) ;
static void LCD_ShiftCursorUp ( void ) ;
static void LCD_ShiftCursorDown ( void ) ;
static void LCD_Wait ( unsigned int ) ;
static void LCD_SendData ( char ) ;
static void LCD_SendCommand ( char ) ;

/* Private variables ************************************************/
static uint8_t row ;
static uint8_t column ;
/*********************************************************************
 * Function: bool LCD_Initialize(void);
 *
 * Overview: Initializes the LCD screen.  Can take several hundred
 *           milliseconds.
 *
 * PreCondition: none
 *
 * Input: None
 *
 * Output: true if initialized, false otherwise
 *
 ********************************************************************/
bool LCD_Initialize ( void )
{    
    LCD_DATA_LAT &= 0xC3FF ;
    LCD_DATA_TRIS &= 0xC3FF;
    // Control signal data pins
    LCD_RWSignal_Clear ( ) ; // LCD R/W signal
    LCD_RSSignal_Clear ( ) ; // LCD RS signal
    LCD_EnableSignal_Clear ( ) ;     // LCD E signal

    // Control signal pin direction
    LCD_RSSignal_Output ( )  ;
    LCD_RWSignal_Output ( )  ;
    LCD_EnableSignal_Output ( ) ;

    LCD_EnableSignal_Set ( )  ;
    LCD_Wait ( LCD_STARTUP ) ;
    LCD_Wait ( LCD_STARTUP ) ;
    
    LCD_SendCommand ( LCD_START_UP_COMMAND_1 ) ;
    LCD_SendCommand ( LCD_START_UP_COMMAND_2  ) ;
    
    LCD_Wait ( LCD_S_INSTR ) ;
    
    LCD_SendCommand ( LCD_COMMAND_SET_MODE_4_BIT  ) ;
    LCD_SendCommand ( LCD_COMMAND_CURSOR_OFF  ) ;
    LCD_SendCommand ( LCD_COMMAND_ENTER_DATA_MODE ) ;

    LCD_ClearScreen ( ) ;

    return true ;
}
/*********************************************************************
 * Function: void LCD_PutString(char* inputString, uint16_t length);
 *
 * Overview: Puts a string on the LCD screen.  Unsupported characters will be
 *           discarded.  May block or throw away characters is LCD is not ready
 *           or buffer space is not available.  Will terminate when either a
 *           null terminator character (0x00) is reached or the length number
 *           of characters is printed, which ever comes first.
 *
 * PreCondition: already initialized via LCD_Initialize()
 *
 * Input: char* - string to print
 *        uint16_t - length of string to print
 *
 * Output: None
 *
 ********************************************************************/
void LCD_PutString ( char* inputString , uint16_t length )
{
    while (length--)
    {
        switch (*inputString)
        {
            case 0x00:
                return ;

            default:
                LCD_PutChar ( *inputString++ ) ;
                break ;
        }
    }
}
/*********************************************************************
 * Function: void LCD_PutChar(char);
 *
 * Overview: Puts a character on the LCD screen.  Unsupported characters will be
 *           discarded.  May block or throw away characters is LCD is not ready
 *           or buffer space is not available.
 *
 * PreCondition: already initialized via LCD_Initialize()
 *
 * Input: char - character to print
 *
 * Output: None
 *
 ********************************************************************/
void LCD_PutChar ( char inputCharacter )
{
    switch (inputCharacter)
    {
        case '\r':
            LCD_CarriageReturn ( ) ;
            break ;

        case '\n':
            if (row == 0)
            {
                LCD_ShiftCursorDown ( ) ;
            }
            else
            {
                LCD_ShiftCursorUp ( ) ;
            }
            break ;

        case '\b':
            LCD_ShiftCursorLeft ( ) ;
            LCD_PutChar ( ' ' ) ;
            LCD_ShiftCursorLeft ( ) ;
            break ;

        default:
            LCD_SendData ( inputCharacter ) ;
            column++ ;

            if (column == LCD_MAX_COLUMN)
            {
                column = 0 ;
                if (row == 0)
                {
                    LCD_SendCommand ( LCD_COMMAND_ROW_1_HOME ) ;
                    row = 1 ;
                }
                else
                {
                    LCD_SendCommand ( LCD_COMMAND_ROW_0_HOME ) ;
                    row = 0 ;
                }
            }
            break ;
    }
}
/*********************************************************************
 * Function: void LCD_ClearScreen(void);
 *
 * Overview: Clears the screen, if possible.
 *
 * PreCondition: already initialized via LCD_Initialize()
 *
 * Input: None
 *
 * Output: None
 *
 ********************************************************************/
void LCD_ClearScreen ( void )
{
    LCD_SendCommand ( LCD_COMMAND_CLEAR_SCREEN ) ;
    LCD_SendCommand ( LCD_COMMAND_RETURN_HOME ) ;

    row = 0 ;
    column = 0 ;
}


/*******************************************************************/
/*******************************************************************/
/* Private Functions ***********************************************/
/*******************************************************************/
/*******************************************************************/
/*********************************************************************
 * Function: static void LCD_CarriageReturn(void)
 *
 * Overview: Handles a carriage return
 *
 * PreCondition: already initialized via LCD_Initialize()
 *
 * Input: None
 *
 * Output: None
 *
 ********************************************************************/
static void LCD_CarriageReturn ( void )
{
    if (row == 0)
    {
        LCD_SendCommand ( LCD_COMMAND_ROW_0_HOME ) ;
    }
    else
    {
        LCD_SendCommand ( LCD_COMMAND_ROW_1_HOME  ) ;
    }
    column = 0 ;
}
/*********************************************************************
 * Function: static void LCD_ShiftCursorLeft(void)
 *
 * Overview: Shifts cursor left one spot (wrapping if required)
 *
 * PreCondition: already initialized via LCD_Initialize()
 *
 * Input: None
 *
 * Output: None
 *
 ********************************************************************/
static void LCD_ShiftCursorLeft ( void )
{
    uint8_t i ;

    if (column == 0)
    {
        if (row == 0)
        {
            LCD_SendCommand ( LCD_COMMAND_ROW_1_HOME  ) ;
            row = 1 ;
        }
        else
        {
            LCD_SendCommand ( LCD_COMMAND_ROW_0_HOME ) ;
            row = 0 ;
        }

        //Now shift to the end of the row
        for (i = 0 ; i < ( LCD_MAX_COLUMN - 1 ) ; i++)
        {
            LCD_ShiftCursorRight ( ) ;
        }
    }
    else
    {
        column-- ;
        LCD_SendCommand ( LCD_COMMAND_MOVE_CURSOR_LEFT ) ;
    }
}
/*********************************************************************
 * Function: static void LCD_ShiftCursorRight(void)
 *
 * Overview: Shifts cursor right one spot (wrapping if required)
 *
 * PreCondition: already initialized via LCD_Initialize()
 *
 * Input: None
 *
 * Output: None
 *
 ********************************************************************/
static void LCD_ShiftCursorRight ( void )
{
    LCD_SendCommand ( LCD_COMMAND_MOVE_CURSOR_RIGHT ) ;
    column++ ;

    if (column == LCD_MAX_COLUMN)
    {
        column = 0 ;
        if (row == 0)
        {
            LCD_SendCommand ( LCD_COMMAND_ROW_1_HOME  ) ;
            row = 1 ;
        }
        else
        {
            LCD_SendCommand ( LCD_COMMAND_ROW_0_HOME ) ;
            row = 0 ;
        }
    }
}
/*********************************************************************
 * Function: static void LCD_ShiftCursorUp(void)
 *
 * Overview: Shifts cursor up one spot (wrapping if required)
 *
 * PreCondition: already initialized via LCD_Initialize()
 *
 * Input: None
 *
 * Output: None
 *
 ********************************************************************/
static void LCD_ShiftCursorUp ( void )
{
    uint8_t i ;

    for (i = 0 ; i < LCD_MAX_COLUMN ; i++)
    {
        LCD_ShiftCursorLeft ( ) ;
    }
}
/*********************************************************************
 * Function: static void LCD_ShiftCursorDown(void)
 *
 * Overview: Shifts cursor down one spot (wrapping if required)
 *
 * PreCondition: already initialized via LCD_Initialize()
 *
 * Input: None
 *
 * Output: None
 *
 ********************************************************************/
static void LCD_ShiftCursorDown ( void )
{
    uint8_t i ;

    for (i = 0 ; i < LCD_MAX_COLUMN ; i++)
    {
        LCD_ShiftCursorRight ( ) ;
    }
}
/*********************************************************************
 * Function: static void LCD_Wait(unsigned int B)
 *
 * Overview: A crude wait function that just cycle burns
 *
 * PreCondition: None
 *
 * Input: unsigned int - artibrary delay time based on loop counts.
 *
 * Output: None
 *
 ********************************************************************/
static void LCD_Wait ( unsigned int delay )
{
    while (delay)
    {
        delay-- ;
    }
}
/*********************************************************************
 * Function: void LCD_CursorEnable(bool enable)
 *
 * Overview: Enables/disables the cursor
 *
 * PreCondition: None
 *
 * Input: bool - specifies if the cursor should be on or off
 *
 * Output: None
 *
 ********************************************************************/
void LCD_CursorEnable ( bool enable )
{
    if (enable == true)
    {
        LCD_SendCommand ( LCD_COMMAND_CURSOR_ON ) ;
    }
    else
    {
        LCD_SendCommand ( LCD_COMMAND_CURSOR_OFF) ;
    }
}
/*********************************************************************
 * Function: static void LCD_SendData(char data)
 *
 * Overview: Sends data to LCD
 *
 * PreCondition: None
 *
 * Input: char - contains the data to be sent to the LCD
 *
 * Output: None
 *
 ********************************************************************/
static void LCD_SendData ( char data )
{
    //Split data into nibbles 
    int data_shift = data << 8;
    int upper_nibble = data_shift & 0xF000;
    upper_nibble = upper_nibble >>2;
    int lower_nibble = data_shift & 0x0F00;
    lower_nibble = lower_nibble <<2;
    
    LCD_RWSignal_Clear ( ) ;
    LCD_RSSignal_Set ( ) ;
    LCD_DATA_LAT &= 0xC3FF ;
    LCD_DATA_LAT |= upper_nibble ;
    LCD_Wait( LCD_S_INSTR);
    LCD_EnableSignal_Set ( ) ;
    LCD_Wait( LCD_S_INSTR);
    
    LCD_EnableSignal_Clear ( ) ;
    LCD_DATA_LAT &= 0xC3FF ;
    LCD_DATA_LAT |= lower_nibble ;
    LCD_Wait( LCD_S_INSTR);
    LCD_EnableSignal_Set ( ) ;
    LCD_Wait( LCD_S_INSTR);
    LCD_EnableSignal_Clear ( ) ;
    LCD_RSSignal_Clear ( ) ;
 
    LCD_Wait ( LCD_S_INSTR ) ;
}
/*********************************************************************
 * Function: static void LCD_SendCommand(char data)
 *
 * Overview: Sends command to LCD
 *
 * PreCondition: None
 *
 * Input: char - contains the command to be sent to the LCD
 *        unsigned int - has the specific delay for the command
 *
 * Output: None
 *
 ********************************************************************/
static void LCD_SendCommand ( char command )
{
    int cmd_shift = command << 8;
    int upper_nibble = cmd_shift & 0xF000;
    int lower_nibble = cmd_shift & 0x0F00;
    upper_nibble = upper_nibble >>2;
    lower_nibble = lower_nibble <<2;
    
    LCD_RWSignal_Clear ( ) ;
    LCD_RSSignal_Clear ( ) ;
    LCD_DATA_LAT &= 0xC3FF ;
    LCD_DATA_LAT |= upper_nibble ;
    
    LCD_Wait( LCD_S_INSTR);
    LCD_EnableSignal_Set ( ) ;
    LCD_Wait( LCD_S_INSTR);
    LCD_EnableSignal_Clear ( ) ;
    LCD_Wait( LCD_S_INSTR);
    LCD_Wait( LCD_S_INSTR);
    
    LCD_RWSignal_Clear ( ) ;
    LCD_RSSignal_Clear ( ) ;
    LCD_DATA_LAT &= 0xC3FF ;  
    LCD_DATA_LAT |= lower_nibble ;
   
    LCD_EnableSignal_Set ( ) ;
    LCD_Wait( LCD_S_INSTR);
    
    LCD_EnableSignal_Clear ( ) ;
    LCD_EnableSignal_Clear ( ) ;
    LCD_Wait( LCD_S_INSTR);
    LCD_Wait( LCD_S_INSTR);
}