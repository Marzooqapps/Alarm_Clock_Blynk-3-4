/**
 * @file Lab3_4.c
 * @author your name (you@domain.com), Jonathan Valvano, Matthew Yu (matthewjkyu@gmail.com)
 *    <TA NAME and LAB SECTION # HERE>
 * @brief
 *    A default main file for running lab3 and 4.
 *    Feel free to edit this to match your specifications.
 *
 *    For these two labs, the student must implement an alarm clock with various 
 *    functions (lab 3) and then integrate it with a remote server, Blynk (lab 4). 
 *    This assignment is open ended, so students must plan the features of their 
 *    alarm clock (besides some base required features), design drivers for peripherals 
 *    used by the clock (ST7735 drawing routines, switch debounce drivers, and so forth), 
 *    and integrate it all together to have a functioning device. Good luck!
 * 
 * @version 0.2
 * @date 2022-09-12 <REPLACE WITH DATE OF LAST REVISION>
 *
 * @copyright Copyright (c) 2022
 * @note 
 *    We suggest following the pinouts provided by the 
 *    EE445L_Baseline_Schematic_Guide.pdf, found in the resources folder.
 *    Warning. Initial code for the RGB driver creates bright flashing lights. 
 *    Please remove this code and do not run if you have epilepsy.
 */

/* This example accompanies the book
   "Embedded Systems: Real Time Interfacing to Arm Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2021

 Copyright 2022 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains
 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */

/** File includes. */
#include <stdio.h>
#include <stdint.h>
#include <math.h>

/* Register definitions. */
#include "./inc/tm4c123gh6pm.h"
/* Clocking. */
#include "./inc/PLL.h"
/* Clock delay and interrupt control. */
#include "./inc/CortexM.h"
/* Initialization of all the pins. */
#include "./inc/Unified_Port_Init.h"
/* Talking to PC via UART. */
#include "./inc/UART.h"
/* Talking to Blynk via the ESP8266. */
#include "./inc/Blynk.h"
/* ST7735 display. */
#include "./inc/ST7735.h"
/* Add whatever else you need here! */
#include "./lib/RGB/RGB.h"
#include "./inc/Timer0A.h"
#include "./inc/EdgeInterruptPortF.h"
#include "./inc/Timer2A.h"
#include "./inc/SysTickInts.h"
#include "InterruptPortC.h"
#include "./inc/PWM.h"

/* TODO: enable this for lab 4. */

#define LAB_4 true

/* TODO: We suggest using the ./inc/ADCSWTrigger.h and the ./inc/TimerXA.h headers. */

/** MMAP Pin definitions. */
#define PF0   (*((volatile uint32_t *)0x40025004)) // Left Button
#define PF1   (*((volatile uint32_t *)0x40025008)) // RED LED
#define PF2   (*((volatile uint32_t *)0x40025010)) // BLUE LED
#define PF3   (*((volatile uint32_t *)0x40025020)) // GREEN LED
#define PF4   (*((volatile uint32_t *)0x40025040)) // Right Button
	
/** Global Variable. */
uint32_t Time;
uint16_t Seconds;
uint16_t Minutes;
uint16_t Hour;
uint16_t AlarmHours;
uint16_t AlarmMinutes; 
uint16_t Call=0;

blynk_info_t blynk_data;

bool Alarm =false;
bool AlarmChange =false;

enum TimeType {hour, min, sec}timeType;

/** Function declarations. */
/**
 * @brief DelayWait10ms delays the current process by n*10ms. Approximate.
 * 
 * @param n Number of times to delay 10ms.
 * @note Based on a 80MHz clock.
 */
void DelayWait10ms(uint32_t n);

/**
 * @brief Blocks the current process until PF4 (Left Button <=> SW1) is pressed.
 */
void Pause(void);

/**
 *@brief Gets called every second by Timer0A Interrupt Handler every second, increments global variable second
 *changes the global variable time to current time
 */
void changeTime(void);

/**
 *@brief Gets called by the IS handler for PF0, increments the global variable Minutes;
 */
void changeMinutes(void);

/**
 *@brief Gets called by the IS handler for PF4, increments the global variable Hours;
 */
void changeHours(void);

/**
 *@brief Displays the time on the ST7735 LCD, displays differenty time types (hours, mins, secs) at different locations on the LCD
 *@param Takes in enum TimeType and sets the cursor on the LCD accoring to time type;
 */
void displayTime(uint16_t, enum TimeType);

void displayAlarmTime(uint16_t, uint16_t);


void changeAlarmMinutes(void);
void changeAlarmHours(void);
void playSound(void);
void displayInit(void);

/** Entry point. */
  int main(void) {
    DisableInterrupts();

    /* Interrupts currently being used:
        Timer0A, pri7 - RGB flashing
        Timer2A, pri4 - ESP8266 sampling
				Timer3A, pri2 - EdgeInterruptDebouncePortF
        UART0, pri7 - PC communication
        UART5 (lab4), pri2 - ESP8266 communication
				Systick, pri3 - Clock logic
    */

    /* PLL Init. */
    PLL_Init(Bus80MHz);

    /* Allow us to talk to the PC via PuTTy! Check device manager to see which
     COM serial port we are on. The baud rate is 115200 chars/s. */
    UART_Init();
	
		/* Initialize all ports. */
    Unified_Port_Init();
    
    /* Start up display. */
    ST7735_InitR(INITR_REDTAB);

    
    /* Start RGB flashing. WARNING! BRIGHT FLASHING COLORS. DO NOT RUN IF YOU HAVE EPILEPSY. */
    RGBInit();

		//Enable SysTick so it interrupts every second and has a priority of 2
		SysTick_Init(16000000 ,changeTime);
		
		//Enable PortF Interrupt
		EdgePortF_Init(changeHours, changeMinutes);
		
		//Enabe PortC Interrupt
		Port_C_Init();
		
		//Enable PWM
		PWM0A_Init(40000, 0);
		
    /* Allows any enabled timers to begin running. */
    EnableInterrupts();

    /* Print starting message to the PC and the ST7735. */
    ST7735_FillScreen(ST7735_BLACK);
    ST7735_SetCursor(0, 0);
    ST7735_OutString(
        "ECE445L Lab 3 & 4.\n"
        "Press SW1 to start.\n");
    UART_OutString(
        "ECE445L Lab 3 & 4.\r\n"
        "Press SW1 to start.\r\n");
    // Temporayily remove
		Pause();
		
    /* Stop RGB and turn off any on LEDs. */
    RGBStop();
    PF1 = 0;
    PF2 = 0;
    PF3 = 0;
		
    /* Reset screen. */
    ST7735_FillScreen(ST7735_BLACK);
    ST7735_SetCursor(0, 0);
    ST7735_OutString("Starting...\n");
    UART_OutString("Starting...\r\n");
		
		//Initialize Screen
		displayInit();
    /* Setup ESP8266 to talk to Blynk server. See blynk.h for what each field does. */
    // TODO: enable this for lab 4
    #if LAB_4
          #define USE_TIMER_INTERRUPT true
          blynk_init("EE-IOT-Platform-03", "g!TyA>hR2JTy", "C5qM0snSecS6kV7SEg0etElkJi4i6kVj", USE_TIMER_INTERRUPT);
          #undef USE_TIMER_INTERRUPT
    #endif
		
		//
		bool AlarmEnabled;
		blynk_info_t blynk_outHours = {70,0,0};
		blynk_info_t blynk_outMinutes = {71,0,0};
		blynk_info_t blynk_outSeconds = {72,0,0};
    while (1) {
        /* TODO: Write your code here! */
				uint16_t alarmMinutes;
				uint16_t alarmHours;
				DisableInterrupts();
				uint32_t myTime = Time;
				alarmMinutes = AlarmMinutes;
				alarmHours =  AlarmHours;
				EnableInterrupts();
			
				//ST7735_FillScreen(ST7735_BLACK);
				if(myTime%100 == 0){
					ST7735_FillRect(61, 04, 30, 60, ST7735_BLACK);
				}
				displayTime(myTime/10000, hour);
				displayTime((myTime/100)%100, min);
				displayTime(myTime%100, sec);
				if(Alarm){
					do{
						ST7735_SetCursor(2,12);
						ST7735_OutString("Alarm Enabled  ");
					} while(false);
					displayAlarmTime(alarmMinutes, alarmHours);
					if(alarmMinutes == (myTime/100)%100 && alarmHours == myTime/10000){
							PWM0A_Duty(20000);
								
					}
				}
				if(!Alarm){
					do{
						ST7735_SetCursor(2,12);
						ST7735_OutString("Alarm Disabled");
					} while(false);
					PWM0A_Duty(0);
				}
				
				//Lab 4 get data from blynk periodically to directly change alarm time
				if(blynk_get_data_from_queue(&blynk_data)){
					
					DisableInterrupts();
					
					if(blynk_data.pin_number==1){
							Hour = blynk_data.float_value;
					}
					else if(blynk_data.pin_number==2){
							Minutes = blynk_data.float_value;
					}
					else if(blynk_data.pin_number == 3){
							Seconds = blynk_data.integer_value;
					}
					EnableInterrupts();
				}
				
				//Set data to send to Blynk
				blynk_outHours.integer_value = Hour;
				blynk_outMinutes.integer_value = Minutes;
				blynk_outSeconds.integer_value = Seconds;
				
				//Send data to Blynk
				tm4c_to_blynk(blynk_outHours);
				tm4c_to_blynk(blynk_outMinutes);
				tm4c_to_blynk(blynk_outSeconds)
				;
				PF2 ^= 0x02;												//Heartbeat
				
				
        WaitForInterrupt();
    }
    return 1;
}

/** Function Implementations. */
void DelayWait10ms(uint32_t n) {
    uint32_t volatile time;
    while (n){
        time = 727240 * 2 / 91;  // 10msec
        while (time){
            --time;
        }
        --n;
    }
}

void Pause(void) {
    while (PF4 == 0x00) {
        DelayWait10ms(10);
    }
    while (PF4 == 0x10) {
        DelayWait10ms(10);
    }
}
void changeTime(void){ 
		Call++;
		if(Call >=5){
			Seconds++;
			if(Seconds>=60){
				Seconds = 0;
			Minutes++;
			}
			if(Minutes >= 60){
				Minutes = 0;
				Hour++;
			}
			if(Hour >= 24) {
				Hour =0;
			}
			Call =0;
		}
		
	Time = 10000*Hour+100*Minutes+Seconds;
}

void displayTime(uint16_t time, enum TimeType timeType){
			
			//use SNprintf //
			if(timeType == hour){
				ST7735_SetCursor(11,1);
			}
			else if(timeType == min){
				ST7735_SetCursor(11,3);
			}
			else{
				ST7735_SetCursor(11,5);
			}
			ST7735_OutUDec(time);
}

void displayAlarmTime(uint16_t min, uint16_t hours){
		
		//Display hours
		ST7735_SetCursor(17,9);
		ST7735_OutUDec(min);
		//Display  minutes
		ST7735_SetCursor(13,9);
		ST7735_OutUDec(hours);
}

void changeMinutes(void){
		if(AlarmChange){
			changeAlarmMinutes();
		}
		else{
			if(Minutes < 60){
			Minutes++;
		}
		else{
			Minutes = 0;
		}
		}
		
}

void changeHours(void){
		if(AlarmChange){
			changeAlarmHours();
		}
		else{
			if(Hour < 24){
			Hour++;
		}
		else{
			Hour = 0;
		}
		}
		
}

void changeAlarmMinutes(void){
	if(AlarmMinutes < 60){
			AlarmMinutes++;
		}
		else{
			AlarmMinutes = 0;
		}
}

void changeAlarmHours(void){
	if(AlarmHours < 24){
			AlarmHours++;
		}
		else{
			AlarmHours = 0;
		}
}
 

void displayInit(void){

		ST7735_FillScreen(ST7735_BLACK);
		ST7735_SetCursor(2,1);
		ST7735_OutString("Hours:");
		
		ST7735_SetCursor(2,3);
		ST7735_OutString("Minutes:");
	
		ST7735_SetCursor(2,5);
		ST7735_OutString("Seconds:");
	
		ST7735_SetCursor(2,9);
		ST7735_OutString("Alarm Time:");

}