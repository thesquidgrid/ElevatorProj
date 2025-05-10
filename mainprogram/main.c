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
#include <string.h>

#include <ti/devices/msp/msp.h>

#include "clock.h"
#include "LaunchPad.h"

#include "adc.h"
#include "lcd1602.h"
#include "uart.h"

#define END_CHARACTER                                                    ('\0')
#define SIZE_LIMIT                                                         (7)
#define MAX_ATTEMPTS                                                       (3)
#define RETRY_DELAY_MS                                                  (1500)

void msp_printf(char * buffer, unsigned int value);

void config_pb1_interrupt(void);
void config_pb2_interrupt(void);
void GROUP1_IQRHANDLER(void);

bool access_code();
void floor_picker();

bool g_pb1_pressed = false;
bool g_pb2_pressed = false;

int main(void) {
    clock_init_40mhz();
    launchpad_gpio_init();
    dipsw_init();
    I2C_init();
    lcd1602_init();
    led_init();
    seg7_init();
    keypad_init();

    ADC0_init(ADC12_MEMCTL_VRSEL_VDDA_VSSA);

    config_pb1_interrupt();
    config_pb2_interrupt();

    if (access_code())
    {
        led_enable();
        floor_picker();
    }

    NVIC_DisableIRQ(GPIOB_INT_IRQn);
    NVIC_DisableIRQ(GPIOA_INT_IRQn);
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
void config_pb1_interrupt(void)  // Credit to you (aka Prof. Link)
{
    GPIOB->POLARITY31_16 = GPIO_POLARITY31_16_DIO18_RISE;
    GPIOB->CPU_INT.ICLR = GPIO_CPU_INT_ICLR_DIO18_CLR;

    GPIOB->CPU_INT.IMASK = GPIO_CPU_INT_IMASK_DIO18_SET;

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
void config_pb2_interrupt(void)  // Credit to you (aka Prof. Link)
{
    GPIOA->POLARITY15_0 = GPIO_POLARITY15_0_DIO15_RISE;
    GPIOA->CPU_INT.ICLR = GPIO_CPU_INT_ICLR_DIO15_CLR;

    GPIOA->CPU_INT.IMASK = GPIO_CPU_INT_IMASK_DIO15_SET;

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

    do 
    {
        group_gpio_iidx = CPUSS->INT_GROUP[1].IIDX;
        switch (group_gpio_iidx)
        {
            case (CPUSS_INT_GROUP_IIDX_STAT_INT1): //GPIOB
                gpio_mis = GPIOB->CPU_INT.MIS;
                if ((gpio_mis & GPIO_CPU_INT_MIS_DIO18_MASK) == GPIO_CPU_INT_MIS_DIO18_SET)
                {
                    g_pb1_pressed = true;
                    GPIOB->CPU_INT.ICLR = GPIO_CPU_INT_ICLR_DIO18_CLR;
                }
                break;
            case (CPUSS_INT_GROUP_IIDX_STAT_INT0): //GPIOA
                gpio_mis = GPIOA->CPU_INT.MIS;
                if ((gpio_mis & GPIO_CPU_INT_MIS_DIO15_MASK) == GPIO_CPU_INT_MIS_DIO15_SET)
                {
                    g_pb2_pressed = true;
                    GPIOA->CPU_INT.ICLR = GPIO_CPU_INT_ICLR_DIO15_CLR;
                }
                break;
        }
    } while (group_gpio_iidx != 0);
}


bool access_code()
{
    const char correct_code[] = "2222222";  // test code
    char buffer[SIZE_LIMIT + 1];            // +1 for null terminator (not required for comparison)
    uint8_t idx = 0;
    uint8_t wrong_attempts = 0;

    while (1)
    {
        lcd_clear();
        lcd_write_string("Enter Code: ");
        lcd_set_ddram_addr(LCD_LINE2_ADDR);

        // Input loop
        idx = 0;
        while (idx < SIZE_LIMIT)
        {
            uint8_t key = getkey_pressed();

            if (key <= 15)
            {
                char c = (key < 10) ? ('0' + key) : ('A' + (key - 10));
                buffer[idx++] = c;
                lcd_write_char(c);
            }

            wait_no_key_pressed();
            msec_delay(10);
        }

        // Check correctness
        bool correct = true;
        for (int i = 0; i < SIZE_LIMIT; i++)
        {
            if (buffer[i] != correct_code[i])
            {
                correct = false;
                break;
            }
        }

        if (correct)
        {
            lcd_clear();
            lcd_write_string("-- WELCOME --");
            msec_delay(1000);
            return true;
        }

        // Incorrect attempt
        wrong_attempts++;
        lcd_clear();

        if (wrong_attempts >= MAX_ATTEMPTS)
        {
            lcd_write_string("SECURITY ALERT");
            msec_delay(2000);
            return false;
        }

        lcd_write_string("-- TRY AGAIN --");
        msec_delay(RETRY_DELAY_MS);
    }
}


void floor_picker()
{
    uint16_t adc_value = 0;
    uint16_t adc_start = 0;
    uint16_t adc_end = 0;
    char direction = ' ';
    uint16_t previous_floor = 99;

    lcd_clear();
    lcd_set_ddram_addr(LCD_LINE1_ADDR);
    lcd_write_string("DIRECTION?");
    lcd_set_ddram_addr(LCD_LINE2_ADDR);
    lcd_write_string("PB1:UP PB2:DOWN");

    while (direction == ' ')
    {
        if (g_pb1_pressed)
        {
            g_pb1_pressed = false;
            direction = 'U';
            adc_start = 6;
            adc_end = 10;
        }
        else if (g_pb2_pressed)
        {
            g_pb2_pressed = false;
            direction = 'D';
            adc_start = 0;
            adc_end = 4;
        }
    }

    lcd_clear();
    lcd_write_string("Select Floor");
    msec_delay(500);
    lcd_clear();

    while (1)
    {
        adc_value = ADC0_in(7);
        uint16_t range = adc_end - adc_start + 1;
        uint16_t selected_floor = adc_start + ((uint32_t)adc_value * range) / 4096;

        if (selected_floor != previous_floor)
        {
            previous_floor = selected_floor;

            lcd_clear();
            lcd_set_ddram_addr(LCD_LINE1_ADDR);
            lcd_write_string("FLOOR: ");
            if (selected_floor == 0)
                lcd_write_string("B");
            else
                lcd_write_byte(selected_floor);

            lcd_set_ddram_addr(LCD_LINE2_ADDR);
            lcd_write_string("PB1:CONFIRM");
        }

        if (g_pb1_pressed)
        {
            g_pb1_pressed = false;

            lcd_clear();
            lcd_write_string("Traveling to...");
            lcd_set_ddram_addr(LCD_LINE2_ADDR);
            if (selected_floor == 0)
                lcd_write_string("Basement");
            else
                lcd_write_byte(selected_floor);

            leds_off();
            msec_delay(250);

            if (direction == 'D')  // Going DOWN
            {
                // LED 1 = floor 5 (main floor)
                led_on(1);
                msec_delay(300);
                led_off(1);

                for (int floor = 4; floor >= (int)selected_floor; floor--)
                {
                    int led = 5 - floor + 1;  // Floor 4 = LED 2, Floor 3 = LED 3, etc.
                    led_on(led);
                    msec_delay(300);
                    led_off(led);
                }

                if (selected_floor == 0)
                {
                    led_on(6);  // Basement LED
                    msec_delay(400);
                }
            }
            else if (direction == 'U')  // Going UP
            {
                // LED 9 = floor 5 (main floor)
                led_on(9);
                msec_delay(300);
                led_off(9);

                for (int floor = 6; floor <= (int)selected_floor; floor++)
                {
                    int led = 11 - floor;  // Floor 6 = LED 8, Floor 10 = LED 4
                    led_on(led);
                    msec_delay(300);
                    led_off(led);
                }
            }

            msec_delay(300);
            leds_off();
            return;
        }
    }
}







