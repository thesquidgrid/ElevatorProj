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

#include "ti/devices/msp/peripherals/hw_iomux.h"

#include "ti/devices/msp/peripherals/m0p/hw_cpuss.h"

#include <iso646.h>
#include <stdio.h>

#include <stdbool.h>

#include <stdlib.h>

#include <ti/devices/msp/msp.h>

#include "clock.h"

#include "LaunchPad.h"

#include "adc.h"

#include "lcd1602.h"

#include "uart.h"

#define END_CHARACTER ('\0')
#define SIZE_LIMIT (7)
#define MAX_ATTEMPTS (3)
#define RETRY_DELAY_MS (1500)
#define DEBOUNCE 15

int VALIDCODES[] = {
  0000,
  1423,
  7512,
  2310,
  1234,
  2534,
  9902
};
char EMPLOYEE_NAMES[][16] = {
  "Sophia Buchman",
  "Rusquel Ramirez",
  "Bruce Link",
  "Tobin Peterson",
  "Lewis Doyle",
  "Amy Winehouse",
  "ADMIN"
};

bool g_pb1_pressed = false;
bool g_pb2_pressed = false;
bool g_up_pressed = false;
bool g_down_pressed = false;
volatile uint32_t g_current_tick_count = 0;
volatile uint8_t five_seconds_elapsed = 0;


void msp_printf(char * buffer, unsigned int value);
void GROUP1_IRQHandler(void);
int8_t padPress(bool displayInput);
void access_code();
uint16_t keypad_input();
uint16_t keypad_level();
int string_to_uint16(char string[]);
int * checkIfValid(int16_t code);
bool admin();
void config_pb1_interrupt(void);
void config_pb2_interrupt(void);
void config_up_interrupt();
void config_down_interrupt();
void floor_picker();
void reset_five_sec_timer();
void SysTick_Handler();

#define MSPM0_CLOCK_FREQUENCY (40e6)
#define SYST_TICK_PERIOD (.1)
#define SYST_TICK_PERIOD_COUNT (SYST_TICK_PERIOD * MSPM0_CLOCK_FREQUENCY)

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

  pushButton_init();
  msp_printf("meow", 0);

  ADC0_init(ADC12_MEMCTL_VRSEL_VDDA_VSSA);
  config_up_interrupt();
  config_down_interrupt();

  sys_tick_init(SYST_TICK_PERIOD_COUNT);

  
  //goal: allow 2 people to use the elevator at once.
  // dont want to reinvent the wheel and make a 
  //task handler, so i think i'll use systick
  //    if that doesnt work than i'll make a task manager or something where
  //    the two states have state machines that switch between one another every one second.
  //    the states will be enums and global variables but that really sounds like a pain in the ass to do
 
 
 
  while(1){
       while (!g_up_pressed && !g_down_pressed) {

      }
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
         //put code that puts you backt to the beginning
         bool adminAccess = false;
         while (adminAccess == false) {
            adminAccess = admin();
         }

      } else if(validCode == true) { //continue with program
         lcd_clear();
         lcd_set_ddram_addr(LCD_LINE1_ADDR);
         lcd_write_string("Welcome:");
         lcd_set_ddram_addr(LCD_LINE2_ADDR);
         lcd_write_string(EMPLOYEE_NAMES[validCode_and_position[1]]);
         msec_delay(1000);
         lcd_clear();
         floor_picker();
   } 
   }
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
  GPIOB -> POLARITY31_16 |= GPIO_POLARITY31_16_DIO18_RISE;
  GPIOB -> CPU_INT.ICLR |= GPIO_CPU_INT_ICLR_DIO18_CLR;

  GPIOB -> CPU_INT.IMASK |= GPIO_CPU_INT_IMASK_DIO18_SET;

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
  GPIOA -> POLARITY15_0 |= GPIO_POLARITY15_0_DIO15_RISE;
  GPIOA -> CPU_INT.ICLR |= GPIO_CPU_INT_ICLR_DIO15_CLR;

  GPIOA -> CPU_INT.IMASK |= GPIO_CPU_INT_IMASK_DIO15_SET;

  NVIC_SetPriority(GPIOA_INT_IRQn, 2);
  NVIC_EnableIRQ(GPIOA_INT_IRQn);
}

void config_up_interrupt(void) // Credit to you (aka Prof. Link)
{
  GPIOB -> POLARITY31_16 |= GPIO_POLARITY31_16_DIO22_RISE;
  GPIOB -> CPU_INT.ICLR |= GPIO_CPU_INT_ICLR_DIO22_CLR;

  GPIOB -> CPU_INT.IMASK |= GPIO_CPU_INT_IMASK_DIO22_SET;

  NVIC_SetPriority(GPIOB_INT_IRQn, 2);
  NVIC_EnableIRQ(GPIOB_INT_IRQn);
}
//-----------------------------------------------------------------------------

void config_down_interrupt(void) // Credit to you (aka Prof. Link)
{
  GPIOB -> POLARITY31_16 |= GPIO_POLARITY31_16_DIO26_RISE;
  GPIOB -> CPU_INT.ICLR |= GPIO_CPU_INT_ICLR_DIO26_CLR;

  GPIOB -> CPU_INT.IMASK |= GPIO_CPU_INT_IMASK_DIO26_SET;

  NVIC_SetPriority(GPIOB_INT_IRQn, 2);
  NVIC_EnableIRQ(GPIOB_INT_IRQn);
}
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
      if ((gpio_mis & GPIO_CPU_INT_MIS_DIO22_MASK) == GPIO_CPU_INT_MIS_DIO22_SET) {
        g_up_pressed = true;
        GPIOB -> CPU_INT.ICLR = GPIO_CPU_INT_ICLR_DIO22_CLR;

      }
      if ((gpio_mis & GPIO_CPU_INT_MIS_DIO26_MASK) == GPIO_CPU_INT_MIS_DIO26_SET) {
        g_down_pressed = true;
        GPIOB -> CPU_INT.ICLR = GPIO_CPU_INT_ICLR_DIO26_CLR;

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

void GROUP0_IRQHandler(void) // Credit to you (aka Prof. Link)
{
  uint32_t group_gpio_iidx;
  uint32_t gpio_mis;

  do {
    group_gpio_iidx = CPUSS -> INT_GROUP[0].IIDX;
    switch (group_gpio_iidx) {
    case (CPUSS_INT_GROUP_IIDX_STAT_INT1): //GPIOB
      gpio_mis = GPIOB -> CPU_INT.MIS;
      if ((gpio_mis & GPIO_CPU_INT_MIS_DIO18_MASK) == GPIO_CPU_INT_MIS_DIO18_SET) {
        g_pb1_pressed = true;
        GPIOB -> CPU_INT.ICLR = GPIO_CPU_INT_ICLR_DIO18_CLR;
      }
      if ((gpio_mis & GPIO_CPU_INT_MIS_DIO22_MASK) == GPIO_CPU_INT_MIS_DIO22_SET) {
        g_up_pressed = true;
        GPIOB -> CPU_INT.ICLR = GPIO_CPU_INT_ICLR_DIO22_CLR;

      }
      if ((gpio_mis & GPIO_CPU_INT_MIS_DIO26_MASK) == GPIO_CPU_INT_MIS_DIO26_SET) {
        g_down_pressed = true;
        GPIOB -> CPU_INT.ICLR = GPIO_CPU_INT_ICLR_DIO26_CLR;

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

int8_t padPress(bool displayInput) {
  int8_t ascii = keypad_scan();
  if (ascii != 0x10) {
    msec_delay(200);
    if(displayInput == true){
    hex_to_lcd(ascii);
  }
  }
  
  wait_no_key_pressed();
  return ascii;
}

uint16_t keypad_level() {

  bool flag = false;
  int8_t count = 0;
  int8_t ascii = 0;
  uint8_t val = 0;
  while (flag == false) {
    if (count < 1) {
      ascii = padPress(false);
      msec_delay(DEBOUNCE);
      if (ascii != 0x10) {
        count++;
      }
    } else {
      flag = true;
    }
  }
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
      ascii = padPress(true);
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
  int i = 0;
  while (((i < sizeof(VALIDCODES)) && (code_is_valid == false))) {
    if (VALIDCODES[i] == code) {
      code_is_valid = true;
    }
    i++;
  }

  int * validCode_and_position = (int * ) malloc(2 * sizeof(int));

  validCode_and_position[0] = code_is_valid;
  validCode_and_position[1] = i - 1;
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

void floor_picker() {
  char direction = ' ';
  bool floor_selected = false;

  // Wait for direction selection

  lcd_clear();
  lcd_write_string("Select Floor");
  msec_delay(500);


  msec_delay(50);
  lcd_clear();
  lcd_set_ddram_addr(LCD_LINE1_ADDR);
  lcd_write_string("FLOOR: ");
  int16_t previous_floor = -1;
  int16_t selected_floor = -2;
  SysTick_Handler();
  reset_five_sec_timer();
  uint8_t ascii = 0;
  while (!floor_selected) {
    
    ascii = padPress(false);
    msec_delay(DEBOUNCE);
    if (ascii != 0x10) {
       selected_floor = ascii;
    }

    if ((selected_floor != previous_floor) && (selected_floor < 0x11) && (selected_floor != -2)) {
      previous_floor = selected_floor;
      
      if (selected_floor == 0) {
        lcd_set_ddram_addr(LCD_LINE1_ADDR + LCD_CHAR_POSITION_8);
        lcd_write_char('B');
      } else{
        lcd_set_ddram_addr(LCD_LINE1_ADDR + LCD_CHAR_POSITION_8);
        hex_to_lcd(ascii);
      }
      reset_five_sec_timer();
    }
    else if ((previous_floor == selected_floor) && (five_seconds_elapsed == true)) {
      {
        g_pb2_pressed = false;
        floor_selected = true;
        lcd_clear();
        lcd_write_string("Traveling to...");
        lcd_set_ddram_addr(LCD_LINE2_ADDR);
        if (selected_floor == 0){
          lcd_write_string("Basement");
        }else{
          lcd_set_ddram_addr(LCD_LINE2_ADDR);  
          lcd_write_string("floor: ");
          lcd_write_byte(selected_floor);
        }
        msec_delay(1500); // Simulate travel
      } 
    } else if(previous_floor != selected_floor) {
        reset_five_sec_timer();
    }

    msec_delay(50); // Smooth responsiveness
  }
}


void SysTick_Handler(void) {
  static uint32_t elapsed_ticks = 0;

  g_current_tick_count++;
  
  if (!five_seconds_elapsed) {
    elapsed_ticks++;
    if (elapsed_ticks >= 50) { // 
      five_seconds_elapsed = true;
      elapsed_ticks = 0;
      
    }
  }
}


void reset_five_sec_timer(void) {
  g_current_tick_count = 0;
  five_seconds_elapsed = 0;        // use uint8_t or uint32_t for ISR-safe flags
  SysTick->VAL = 0;                // reset current countdown to force immediate reload
  SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;  // ensure the timer is running
}