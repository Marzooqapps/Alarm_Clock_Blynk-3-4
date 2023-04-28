#pragma once
#include <stdint.h>
void Port_C_Init(void);
void Port_C_Arm(void);
void GPIOPortC_Handler(void);


// ***************** Timer1A_Init ****************
// Activate Timer1 interrupts to run user task periodically
// Inputs:  task is a pointer to a user function
//          period in units (1/clockfreq)
//          priority 0 (highest) to 7 (lowest)
// Outputs: none
void Timer1A_Init(uint32_t priority);

void Timer1A_Start(void);

void Timer1A_Stop(void);

void Timer1A_Handler(void);
