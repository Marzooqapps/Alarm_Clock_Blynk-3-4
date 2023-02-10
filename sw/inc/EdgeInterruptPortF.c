// EdgeInterruptPortF.c
// Runs on LM4F120 or TM4C123
// Request an interrupt on the falling edge of PF4 (when the user
// button is pressed) and increment a counter in the interrupt.  Note
// that button bouncing is not addressed.
// Daniel Valvano
// August 30, 2019

/* This example accompanies the book
   "Embedded Systems: Introduction to ARM Cortex M Microcontrollers"
   ISBN: 978-1469998749, Jonathan Valvano, copyright (c) 2019
   Volume 1, Program 9.4
   
   "Embedded Systems: Real Time Interfacing to ARM Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2019
   Volume 2, Program 5.6, Section 5.5

 Copyright 2019 by Jonathan W. Valvano, valvano@mail.utexas.edu
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

// user button connected to PF4 (increment counter on falling edge)

#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"

static void (*PF0pressed)(void);
static void (*PF4pressed)(void);

void EdgePortF_Init(void(*task1)(void), void(*task2)(void)){
  
  SYSCTL_RCGCGPIO_R     |= 0x00000020;      // activate clock for Port F
  while((SYSCTL_PRGPIO_R & 0x20)==0){};     // allow time for clock to stabilize
    
  GPIO_PORTF_LOCK_R     = 0x4C4F434B;       // unlock GPIO Port F
  GPIO_PORTF_CR_R       = 0x1F;             // allow changes to PF4-0
  
  GPIO_PORTF_AMSEL_R    = 0x00;             // disable analog on PF
  GPIO_PORTF_PCTL_R     = 0x00000000;       // PCTL GPIO on PF4-0
  GPIO_PORTF_DIR_R      = 0x0E;             // PF4,PF0 in, PF3-1 out
  GPIO_PORTF_AFSEL_R    = 0x00;             // disable alt funct on PF7-0
  GPIO_PORTF_PUR_R      = 0x11;             // enable pull-up on PF0 and PF4
  GPIO_PORTF_DEN_R      = 0x1F;             // enable digital I/O on PF4-0
	
	//Port F is rising edge triggered interrupt on PF0 and PF4
	GPIO_PORTF_IS_R &= ~0x11;									//PF0 and PF4 are edge-sensitive
	GPIO_PORTF_IEV_R |= 0x11;                 //PF0 and PF4 are rising edge
	
	//Arming the interrupt
	GPIO_PORTF_ICR_R = 0x11;      // clear flag4
  GPIO_PORTF_IM_R |= 0x11;      //arm interrupt on PF4 *** No IME bit as mentioned in Book ***
  NVIC_PRI7_R = (NVIC_PRI7_R&0xFF00FFFF)|0x00300000; //  priority 3
  NVIC_EN0_R = 0x40000000;      // (enable interrupt 30 in NVIC  
  
	//Map User function to interrupt
	PF0pressed = task1;
	PF4pressed = task2;
}
void GPIOPortF_Handler(void){
	if(GPIO_PORTF_RIS_R & (~0x10)){	//Check if PF4 caused the interrupt
		GPIO_PORTF_IM_R = 0x10;
		(*PF4pressed)();
	}
  else{				//Else PF0 caused the interrupt
		GPIO_PORTF_IM_R = 0x01;
		(*PF0pressed)();
  }
}


