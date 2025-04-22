//*****************************************************************************
//*****************************    C Source Code    ***************************
//*****************************************************************************
//  DESIGNER NAME:  Sophia Buchman
//
//       LAB NAME:  lab 10 part 2
//
//      FILE NAME:  lab10_p2_main.c
//
//-----------------------------------------------------------------------------
//
// DESCRIPTION:
//    This program runs a menu-driven interface on the MSPM0G3507 microcontroller. 
//    It allows the user to interact via UART to control a 7-segment display, read temperature data, and flash LEDs. 
//    The program loops until the user chooses to exit.
//*****************************************************************************
//*****************************************************************************

//-----------------------------------------------------------------------------
// Loads standard C include files
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <stdbool.h>

#define enter      '\r'
#define backspace  '\b'

#include <ti/devices/msp/msp.h>

#include "clock.h"
#include "LaunchPad.h"

#include "adc.h"
#include "lcd1602.h"
#include "uart.h"

void msp_printf(char * buffer, unsigned int value);
bool is_valid_option(char c);
int celsius_to_fahrenheit(int temp);
void printMenu(void);
void flashLEDS(void);

int main(void) {
   clock_init_40mhz();
   launchpad_gpio_init();

   I2C_init();
   lcd1602_init();
   UART_init(115200);
   led_init();
   seg7_init();
   ADC0_init(ADC12_MEMCTL_VRSEL_INTREF_VSSA);

   char currentChar = ' ';
   uint8_t seg7Counter = 0;
   uint32_t temp = 0;

   seg7_hex(seg7Counter, 0);

   while (currentChar != '4') {
      printMenu();
      
      currentChar = UART_in_char();
      UART_out_char(currentChar);  // Echo input

      while (!is_valid_option(currentChar)) {
         msp_printf("\nInvalid Input. Try again: ", 0);
         currentChar = UART_in_char();
         UART_out_char(currentChar);
      }

      switch (currentChar) {
         case '1':
            seg7Counter++;
            if(seg7Counter > 9){
                seg7Counter = 0;
            }
            seg7_hex(seg7Counter, 0);
            
            break;

         case '2':
            temp = ADC0_in(ADC12_MEMCTL_CHANSEL_CHAN_5);
            temp = thermistor_calc_temperature(temp);
            temp = celsius_to_fahrenheit(temp);

            lcd_set_ddram_addr(LCD_LINE_NUM_1);
            lcd_write_string("Temp: ");
            lcd_write_byte(temp);
            lcd1602_write(LCD_IIC_ADDRESS, 0xDF, LCD_DATA_REG); // Degree symbol
            break;

         case '3':
            seg7_off();
            flashLEDS();
            seg7_hex(seg7Counter, 0);
            
            break;

         case '4':
            msp_printf("\nProgram End\r\n", 0);
            break;
      }
   }

   lcd_clear();
   lcd_set_ddram_addr(LCD_LINE_NUM_1);
   lcd_write_string("Program Done");

   while (1);
}

//-----------------------------------------------------------------------------
// DESCRIPTION:
//    Prints a formatted string via UART using sprintf and UART_out_char.
//
// INPUT PARAMETERS:
//    char* buffer - Format string containing the message to print.
//    unsigned int value - Value to format into the string.
//
// OUTPUT PARAMETERS:
//    none
//
// RETURN:
//    none
//-----------------------------------------------------------------------------
void msp_printf(char * buffer, unsigned int value) {
   char string[80];
   int len = sprintf(string, buffer, value);

   for (int i = 0; i < len; i++) {
      UART_out_char(string[i]);
   }
}

//-----------------------------------------------------------------------------
// DESCRIPTION:
//    Validates if the input character is a valid menu option ('1' to '4').
//
// INPUT PARAMETERS:
//    char c - Character to validate.
//
// OUTPUT PARAMETERS:
//    none
//
// RETURN:
//    bool - true if valid option, false otherwise.
//-----------------------------------------------------------------------------
bool is_valid_option(char c) {
   return c == '1' || c == '2' || c == '3' || c == '4';
}

//-----------------------------------------------------------------------------
// DESCRIPTION:
//    Converts a temperature from Celsius to Fahrenheit.
//    Uses the formula: (temp * 9/5) + 32.
//
// INPUT PARAMETERS:
//    int temp - Temperature in Celsius.
//
// OUTPUT PARAMETERS:
//    none
//
// RETURN:
//    int - Temperature in Fahrenheit.
//-----------------------------------------------------------------------------
int celsius_to_fahrenheit(int temp) {
   return ((temp * 9.0 / 5.0) + 32.0);
}

//-----------------------------------------------------------------------------
// DESCRIPTION:
//    Displays the menu options to the user via UART.
//    Prompts the user to select an option between 1 and 4.
//
// INPUT PARAMETERS:
//    none
//
// OUTPUT PARAMETERS:
//    none
//
// RETURN:
//    none
//-----------------------------------------------------------------------------
void printMenu(void) {
   msp_printf("\r\n======== MENU ========\r\n", 0);
   msp_printf(" 1. Increment 7-Segment Counter\r\n", 0);
   msp_printf(" 2. Show Current Temperature\r\n", 0);
   msp_printf(" 3. Flash LEDs 3 Times\r\n", 0);
   msp_printf(" 4. End Program\r\n", 0);
   msp_printf("Enter your selection: ", 0);
}

//-----------------------------------------------------------------------------
// DESCRIPTION:
//    Flashes all onboard LEDs three times with a delay between each toggle.
//    Used for visual feedback to the user.
//
// INPUT PARAMETERS:
//    none
//
// OUTPUT PARAMETERS:
//    none
//
// RETURN:
//    none
//-----------------------------------------------------------------------------
void flashLEDS(void) {
   led_enable();

   for (int i = 0; i < 3; i++) {
      leds_on(255);
      msec_delay(500);
      leds_off();
      msec_delay(500);
   }

   led_disable();
}