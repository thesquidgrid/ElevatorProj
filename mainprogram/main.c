//*****************************************************************************
//*****************************    C Source Code    ***************************
//*****************************************************************************
//  DESIGNER NAME:  Sophia Buchman & Rusquel Ramirez
//
//       LAB NAME:  Final Project
//
//      FILE NAME:  final_main.c
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

#include "ti/devices/msp/m0p/mspm0g350x.h"

#include "ti/devices/msp/peripherals/hw_gpio.h"

#include "ti/devices/msp/peripherals/m0p/hw_cpuss.h"

#include <stdio.h>

#include <stdbool.h>

#include <stdlib.h>

#include <ti/devices/msp/msp.h>

#include "clock.h"

#include "LaunchPad.h"

#include "adc.h"

#include "lcd1602.h"

#include "uart.h"

#include <string.h>

#define END_CHARACTER ('\0')
#define SIZE_LIMIT (7)
#define MAX_ATTEMPTS (3)
#define RETRY_DELAY_MS (1500)
#define DEBOUNCE 15

#define FLOOR_1_BIT (1 << 0)
#define FLOOR_2_BIT (1 << 1)
#define FLOOR_3_BIT (1 << 2)
#define FLOOR_4_BIT (1 << 3)
#define FLOOR_5_BIT (1 << 4)
#define FLOOR_6_BIT (1 << 5)
#define FLOOR_7_BIT (1 << 6)
#define FLOOR_8_BIT (1 << 7)
#define FLOOR_9_BIT (1 << 8)


bool g_pb1_pressed = false;
bool g_pb2_pressed = false;

void msp_printf(char * buffer, unsigned int value);
void config_pb1_interrupt(void);
void config_pb2_interrupt(void);
void GROUP1_IRQHandler(void);
int8_t padPress(void);
void access_code();
uint16_t keypad_input();
int string_to_uint16(char string[]);
int * checkIfValid(int16_t code);
bool admin();

int main(void) {
   clock_init_40mhz();
   launchpad_gpio_init();
   dipsw_init();
   I2C_init();
   lcd1602_init();
   UART_init(115200);
   led_init();
   seg7_init();
   keypad_init();

   bool finishProgramFlag = false;
   int currentLevel = 7; 

   while (finishProgramFlag == false) {
      uint8_t attempt_count = 0;
      bool validCode = false;

      uint16_t employee_code;
      int * validCode_and_position = (int * ) malloc(2 * sizeof(int));

      lcd_clear();
      while ((attempt_count < MAX_ATTEMPTS) && (validCode == false)) {
         lcd_set_ddram_addr(LCD_LINE1_ADDR);
         lcd_write_string("Employee Number:");
         employee_code = keypad_input();
         validCode_and_position = checkIfValid(employee_code);
         validCode = validCode_and_position[0];

         if (validCode == false) {
            attempt_count++;
            lcd_clear();
            lcd_write_string("WRONG INPUT");
            lcd_set_ddram_addr(LCD_LINE2_ADDR);
            lcd_write_string("TRY AGAIN");
            msec_delay(1000);
            lcd_clear();
         }
      }
      if (attempt_count == MAX_ATTEMPTS) {
         //put code that puts you back to the beginning
         bool adminAccess = false;
         while (adminAccess == false) {
            adminAccess = admin();
         }

      } else { //continue with programW
         lcd_clear();
         lcd_set_ddram_addr(LCD_LINE1_ADDR);
         lcd_write_string("Welcome:");
         lcd_set_ddram_addr(LCD_LINE2_ADDR);
         lcd_write_string(nameFromIndex(validCode_and_position[1]));
         msec_delay(1000);
         lcd_clear();
         lcd_write_string("Up or Down?");
         uint8_t up_or_down = dipsw_read_pos1();
         if(up_or_down == 1){
         
         }

      }

   }

   //access code 

   /*
   set a predetermined "correct" code and "incorrect" code
   keypad presses -> lcd display (set char limit) 
                   (either display one at a time as you type or all at once after limit)
   if INCORRECT
       lcd TRY AGAIN
       wrong counter++
       if wrong counter = 3
           error buzzer
           "WRONG CODE
           SECURITY!!!"
           program ends
   if CORRECT
       "WELCOME"
   */

   //floor picker

   /*
   lcd_string_write ("DIRECTION?");
   lcd_set_address(LINE2)
   lcd_string write("PB1") + UP Arrow(?) + ("PB2") + DOWN Arrow(?)

   if (g_pb1_pressed)
   {
       set_floors to 6 -> 10
       [ function to select the floor]
           potentiometer to pick floor
           (optional) doors close
           Red LED on, Green LED off
           once floor picked, leds count up to that floor
               (optional) if CLOSE button held, doors count to floor and stay closed
                          if OPEN button held, doors do not close and LEDS don't move
           GREEN Led on
           (optional) doors open
           ding sound
           program asks if thhis is the correct floor
               if yes, end program and (optional) close door
               if no, repeat the whole process
   }

   if (g_pb2_pressed) set floors to B -> 4 + repeat process above
   */

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
//     This function configures an interrupt button for push button PB1. It sets
//     the interrupy to trigger on the rising edge, ensure the interrupt bit is 
//     cleared, unmasks the interruptto allow it, and sets the priority and 
//     enables the interrupt in the NVIC.
//
// INPUT PARAMETERS:
//  none
//
// OUTPUT PARAMETERS:
//  none
//
// RETURN:
//  none
// -----------------------------------------------------------------------------
void config_pb1_interrupt(void) // Credit to you (aka Prof. Link)
{
   GPIOB -> POLARITY31_16 = GPIO_POLARITY31_16_DIO18_RISE;
   GPIOB -> CPU_INT.ICLR = GPIO_CPU_INT_ICLR_DIO18_CLR;

   GPIOB -> CPU_INT.IMASK = GPIO_CPU_INT_IMASK_DIO18_SET;

   NVIC_SetPriority(GPIOB_INT_IRQn, 2);
   NVIC_EnableIRQ(GPIOB_INT_IRQn);
}

//-----------------------------------------------------------------------------
// DESCRIPTION:
//     This function configures an interrupt button for push button PB2. It sets
//     the interrupy to trigger on the rising edge, ensure the interrupt bit is 
//     cleared, unmasks the interruptto allow it, and sets the priority and 
//     enables the interrupt in the NVIC.
//
// INPUT PARAMETERS:
//  none
//
// OUTPUT PARAMETERS:
//  none
//
// RETURN:
//  none
// -----------------------------------------------------------------------------
void config_pb2_interrupt(void) // Credit to you (aka Prof. Link)
{
   GPIOA -> POLARITY15_0 = GPIO_POLARITY15_0_DIO15_RISE;
   GPIOA -> CPU_INT.ICLR = GPIO_CPU_INT_ICLR_DIO15_CLR;

   GPIOA -> CPU_INT.IMASK = GPIO_CPU_INT_IMASK_DIO15_SET;

   NVIC_SetPriority(GPIOA_INT_IRQn, 2);
   NVIC_EnableIRQ(GPIOA_INT_IRQn);
}

//-----------------------------------------------------------------------------
// DESCRIPTION:
//     This function serves as the Interrupt Service Routine (ISR) for the
//     the interrupt group 1. It handles interrupts from GPIOA and GPIOB,
//     setting global flags to indicate when push buttons PB1 and PB2 have been
//     pressed.
//
// INPUT PARAMETERS:
//  none
//
// OUTPUT PARAMETERS:
//  none
//
// RETURN:
//  none
// -----------------------------------------------------------------------------
void GROUP1_IRQHandler(void) // Credit to you (aka Prof. Link)
{
   uint32_t group_gpio_iidx;
   uint32_t gpio_mis;

   do {
      group_gpio_iidx = CPUSS -> INT_GROUP[1].IIDX;
      switch (group_gpio_iidx) {
      case (CPUSS_INT_GROUP_IIDX_STAT_INT1): //GPIOB
         gpio_mis = GPIOB -> CPU_INT.MIS;
         if ((gpio_mis & GPIO_CPU_INT_MIS_DIO18_MASK) == GPIO_CPU_INT_MIS_DIO18_SET) {
            g_pb1_pressed = true;
            GPIOB -> CPU_INT.ICLR = GPIO_CPU_INT_ICLR_DIO18_CLR;
         }
         break;
      case (CPUSS_INT_GROUP_IIDX_STAT_INT0): //GPIOA
         gpio_mis = GPIOA -> CPU_INT.MIS;
         if ((gpio_mis & GPIO_CPU_INT_MIS_DIO15_MASK) == GPIO_CPU_INT_MIS_DIO15_SET) {
            g_pb2_pressed = true;
            GPIOA -> CPU_INT.ICLR = GPIO_CPU_INT_ICLR_DIO15_CLR;
         }
         break;
      }
   } while (group_gpio_iidx != 0);
}

// -----------------------------------------------------------------------------
// DESCRIPTION
//    This function scans for a keypad press, converts the key value to its 
//    corresponding hexadecimal representation, and displays it on the LCD. 
//    It ensures no key is pressed before returning the scanned ASCII value.
//
// INPUT PARAMETERS:
//    none
//
// OUTPUT PARAMETERS:
//    none
//
// RETURN:
//    ascii - The ASCII value of the key pressed on the keypad.
// -----------------------------------------------------------------------------

int8_t padPress() {
   int8_t ascii = keypad_scan();
   if (ascii != 0x10) {
      hex_to_lcd(ascii);
      msec_delay(200);
   }
   wait_no_key_pressed();
   return ascii;
}

uint16_t keypad_input() {

   lcd_set_ddram_addr(LCD_LINE2_ADDR + LCD_CHAR_POSITION_7);

   bool flag = false;
   int8_t count = 0;
   int8_t ascii = 0;
   char buffer[5] = {
      0
   };
   char input_char;

   while (flag == false) {

      if (count < 4) {
         ascii = padPress();
         msec_delay(DEBOUNCE);
         if (ascii != 0x10) {
            buffer[count] = ascii + '0'; //convert integer to character and then put it in array.
            count++;
         }
      } else {
         flag = true;
      }
   }

   buffer[count] = '\0';
   int16_t employeeNum = string_to_uint16(buffer);
   return employeeNum;
}

//-----------------------------------------------------------------------------
// DESCRIPTION:
//    Converts a numeric string into an unsigned 16-bit integer.
//
// INPUT PARAMETERS:
//    char string[] - Input string of digits.
//
// OUTPUT PARAMETERS:
//    none
//
// RETURN:
//    int - Parsed integer.
//-----------------------------------------------------------------------------
int string_to_uint16(char string[]) {
   int result = 0;

   while ( * string != NULL) {
      if ( * string >= '0' && * string <= '9') {
         result = result * 10 + ( * string - '0');
      } else {
         return 0; // Invalid character
      }
      string++;
   }
   return result;
}

//return position code was found as well as if it was found in the first place
int * checkIfValid(int16_t code) {
   bool code_is_valid = false;
   int * validCode_and_position = (int * ) malloc(2 * sizeof(int));
   int i = 0;
  
    FILE *fp = fopen("employee-codes.txt", "r");


    if (fp != NULL) {
        // Read each line from the file and store it in the
        // 'line' buffer.
        int counter = 0;
        int num;
        while (fscanf(fp, "%d", &num) != EOF) {
            printf("%d\n", num);
            counter++;
            if(num == code){
                code_is_valid = true;
            }
        }

        fclose(file);
        validCode_and_position[0] = code_is_valid;
        validCode_and_position[1] = i - 1;
    } else {
        // Print an error message to the standard error
        // stream if the file cannot be opened.
        printf(stderr, "Unable to open file!\n");
        validCode_and_position[0] = code_is_valid;
        validCode_and_position[1] = i - 1;
    }

   return validCode_and_position;
}

bool admin() {
   lcd_clear();
   lcd_set_ddram_addr(LCD_LINE1_ADDR);
   lcd_write_string("WAIT FOR ADMIN:");
   int password = keypad_input();
   bool admin_access = false;
   int admin_value_index = (sizeof(VALIDCODES) / sizeof(VALIDCODES[0])) - 1;
   if (password == VALIDCODES[admin_value_index]) {
      admin_access = true;
   }
   return admin_access;
}

char* nameFromIndex(uint8_t index){
    FILE* file = fopen("employee-names.txt", "r");

    // Buffer to store each line of the file.
    char line[256];
    // Check if the file was opened successfully.
    if (file != NULL) {
        // Read each line from the file and store it in the
        // 'line' buffer.
        int counter = 0;
        int i = 0;
        

        do{
            fgets(line, sizeof(line), file)
            printf("%s", line);
            i++;
        } while (i < index) {
            // Print each line to the standard output.
            fgets(line, sizeof(line), file)
            printf("%s", line);
            i++;
        }
        // Close the file stream once all lines have been
        // read.
        fclose(file);
    }
    else {
        // Print an error message to the standard error
        // stream if the file cannot be opened.
        fprintf(stderr, "Unable to open file!\n");
    }
    return line;
}

