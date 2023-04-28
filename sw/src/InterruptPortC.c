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
#include "InterruptPortC.h"

//Globar Variables - start

bool PC4pressed;

extern bool Alarm;
extern bool AlarmChange;

//Global Variables - end


void Port_C_Init(void){ 
  
  SYSCTL_RCGCGPIO_R     |=  0x04;         // Activate clock for Port C
  while((SYSCTL_PRGPIO_R & 0x04) != 0x04){};  // Allow time for clock to start
   
  GPIO_PORTC_PCTL_R     &= ~0xFFFF0000;   // regular GPIO
  GPIO_PORTC_AMSEL_R    &= ~0xF0;         // disable analog function 
  GPIO_PORTC_DIR_R      &= ~0xF0;         // inputs on PC7-PC4
  GPIO_PORTC_AFSEL_R    &= ~0xF0;         // regular port function
  //GPIO_PORTC_PUR_R       =  0xF0;         // enable pull-up on PC7-PC4
  GPIO_PORTC_DEN_R      |=  0xF0;         // enable digital port 
	GPIO_PORTC_IS_R       &= ~0x30;					// PC4 and PC5 are edge triggered
	GPIO_PORTC_IBE_R 			|= 	0x20;					// PC5 triggered by rising and falling edge
	GPIO_PORTC_IEV_R 			|=  0x10;					// PC4 is rising edge
	Port_C_Arm();
	Timer1A_Init(3);
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
	Timer1A_Start();				//Start one shot timer
	if(GPIO_PORTC_RIS_R & (0x10)){	//Check if PC4 caused the interrupt
		GPIO_PORTC_ICR_R = 0x10;
		PC4pressed = true;										// means PC4 was pressed	
	}
	
  else{				//Else PC5 caused the interrupt
		GPIO_PORTF_ICR_R = 0x20;
		PC4pressed = false;										// means PC5 was pressed
  }
}



//////
// ***************** TIMER1A_Init ****************
// Activate TIMER1A interrupts to run user task periodically
// Inputs:  task is a pointer to a user function
//          period in units (1/clockfreq)
//          priority 0 (highest) to 7 (lowest)
// Outputs: none
void Timer1A_Init(uint32_t priority){
  SYSCTL_RCGCTIMER_R |= 0x02;   // 0) activate TIMER1
  //PeriodicTask1 = task;         // user function
  TIMER1_CTL_R = 0x00000000;    // 1) disable TIMER1A during setup
  TIMER1_CFG_R = 0x00000000;    // 2) configure for 32-bit mode
  TIMER1_TAMR_R = 1;					  // 3) One-shot mode
  TIMER1_TAILR_R = 160000;;    // 4) reload value
  TIMER1_TAPR_R = 0;            // 5) bus clock resolution
  TIMER1_ICR_R = 0x00000001;    // 6) clear TIMER1A timeout flag
  TIMER1_IMR_R = 0x00000001;    // 7) arm timeout interrupt
  NVIC_PRI5_R = (NVIC_PRI5_R&0xFFFF00FF)|(priority<<13); // priority 
// interrupts enabled in the main program after all devices initialized
// vector number 37, interrupt number 21
	//Timer1A_Start();
}

void Timer1A_Handler(void){
  if (PC4pressed){
		Alarm = !Alarm;
	}
	
	else{
		
		AlarmChange = !AlarmChange;
	}
	
	Port_C_Arm();
	Timer1A_Stop();
}
void Timer1A_Start(void){
	NVIC_EN0_R = 1<<21;           // 9) enable IRQ 21 in NVIC
  TIMER1_CTL_R = 0x00000001;    // 10) enable TIMER1A
}

void Timer1A_Stop(void){
  NVIC_DIS0_R = 1<<21;        // 9) disable IRQ 21 in NVIC
  TIMER1_CTL_R = 0x00000000;  // 10) disable timer1A
}