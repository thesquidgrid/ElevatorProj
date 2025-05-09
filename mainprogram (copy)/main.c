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

#include <stdio.h>

#include <stdbool.h>

#include <stdlib.h>

#include <ti/devices/msp/msp.h>

#include "clock.h"

#include "LaunchPad.h"

#include "adc.h"

#include "lcd1602.h"

#include "uart.h"

#include "uart.h"

#include "pthread.h"

bool g_pb1_pressed = false;
bool g_pb2_pressed = false;
bool g_up_pressed = false;
bool g_down_pressed = false;

void msp_printf(char * buffer, unsigned int value);

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
   
   while (1);
}
