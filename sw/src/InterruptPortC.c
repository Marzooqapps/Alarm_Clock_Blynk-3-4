// InterruptPortC.c
// Runs on TM4C123
// Request an interrupt on the falling and rising edge of PC5 and 
// rising edge of PC4.
// Marzooq Shah
// February 20, 2023

#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <stdbool.h>
#include "../inc/tm4c123gh6pm.h"

//Globar Variables - start

bool PC4pressed;

extern bool Alarm;
extern bool AlarmChange;

//Global Variables - end

void Timer3B_Start(void){
  SYSCTL_RCGCTIMER_R |= 0x08;   // 0) activate timer3

  TIMER3_CTL_R = 0x00000000;    // 1) disable timer3A during setup
  TIMER3_CFG_R = 0x00000000;    // 2) configure for 32-bit mode
  TIMER3_TAMR_R = 0x00000001;   // 3) 1-SHOT mode
  TIMER3_TAILR_R = 160000;    	// 4) reload value
  TIMER3_TAPR_R = 0;            // 5) bus clock resolution
  TIMER3_ICR_R = 0x00000002;    // 6) clear timer3B timeout flag
  TIMER3_IMR_R = 0x00000001;    // 7) arm timeout interrupt
  NVIC_PRI9_R = (NVIC_PRI9_R&0x00FFFFFF)|(4<<5); // priority is 4
// interrupts enabled in the main program after all devices initialized
// vector number 39, interrupt number 23
  NVIC_EN1_R = 1<<(36-32);      // 9) enable IRQ 36 in NVIC
  TIMER3_CTL_R = 0x00000002;    // 10) enable timer3B
	
}

void Port_C_Init(void){ 
  
  SYSCTL_RCGCGPIO_R     |=  0x04;         // Activate clock for Port C
  while((SYSCTL_PRGPIO_R & 0x04) != 0x04){};  // Allow time for clock to start
   
  GPIO_PORTC_PCTL_R     &= ~0xFFFF0000;   // regular GPIO
  GPIO_PORTC_AMSEL_R    &= ~0xF0;         // disable analog function 
  GPIO_PORTC_DIR_R      &= ~0xF0;         // inputs on PC7-PC4
  GPIO_PORTC_AFSEL_R    &= ~0xF0;         // regular port function
  GPIO_PORTC_PUR_R       =  0xF0;         // enable pull-up on PC7-PC4
  GPIO_PORTC_DEN_R      |=  0xF0;         // enable digital port 
	GPIO_PORTC_IS_R       &= ~0x30;					// PC4 and PC5 are edge triggered
	GPIO_PORTC_IBE_R 			|= 	0x20;					// PC5 triggered by rising and falling edge
	GPIO_PORTC_IEV_R 			|=  0x10;					// PC4 is rising edge
	
  }  

void Port_C_Arm(void){
	
	GPIO_PORTC_ICR_R = 0x30;      // clear falg
  GPIO_PORTC_IM_R |= 0x30;      //arm interrupt on PC4 and PC5 *** No IME bit as mentioned in Book ***
  NVIC_PRI0_R = (NVIC_PRI0_R&0xFF00FFFF)|0x00600000; //  priority 6
  NVIC_EN0_R = 0x00000004;      // enable interrupt 2 in NVIC
}	

void GPIOPortC_Handler(void){
	//start a timer here and then the timer interrupt gets your job done.
	GPIO_PORTC_IM_R &= ~0x30;
	Timer3B_Start();				//Start one shot timer
	if(GPIO_PORTC_RIS_R & (~0x10)){	//Check if PC4 caused the interrupt
		GPIO_PORTC_ICR_R = 0x10;
		PC4pressed = true;										// means PC4 was pressed	
	}
	
  else{				//Else PC5 caused the interrupt
		GPIO_PORTF_ICR_R = 0x20;
		PC4pressed = false;										// means PC5 was pressed
  }
}

void Timer3B_Handler(void){
	if (PC4pressed){
		Alarm = !Alarm;
	}
	
	else{
		
		AlarmChange = true;
	}
	
	
}