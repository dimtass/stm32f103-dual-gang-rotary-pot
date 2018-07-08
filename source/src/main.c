/**
 * This is a demo for the dual gang non-stop rotary pot
 * 
 * The pot has the following connections:
 * 
 *         (1)
 *          |
 *       _/---\_
 *      | |   | |
 * (2)->| |   | |
 *      |_|   |_|<-(3)
 *       \_____/
 *          |
 *         (4)
 * 
 * (1) +Vcc
 * (2) Resistor 1 wiper
 * (3) Resistor 2 wiper
 * (4) GND
 * 
 * Note:
 * 	On this potensiometer the wipers have 90 degrees phase,
 * 	therefore the code is only specific to this type of
 * 	potensiometer. Also the ADC1 is connected on the wiper
 * 	that is 90 degrees behind the other one, which is connected
 * 	on the ADC2.
 * 
 *  Created on: Jul 5, 2018
 *      Author: Dimitris Tassopoulos
*/


#include "platform_config.h"
#include "hw_config.h"
#include "rotary_cont_pot.h"

/* Declare glb struct and initialize buffers */
struct tp_glb glb;

DECLARE_UART_DEV(dbg_uart, USART1, 115200, 256, 10, 1);

void main_loop(void)
{
	/* 1 ms timer */
	if (glb.tmr_1ms) {
		glb.tmr_1ms = 0;

		dev_uart_update(&dbg_uart);
	}
	if (glb.adc1_ready && glb.adc2_ready) {
		glb.adc1_ready = 0;
		glb.adc2_ready = 0;
		rcp_set_update_adc_values(0, glb.adc1_val, glb.adc2_val);
	}
}

int main(void)
{
	if (SysTick_Config(SystemCoreClock / 1000)) {
		/* Capture error */
		while (1);
	}

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	/* System Clocks Configuration */
	RCC_Configuration();
	/* NVIC Configuration */
	NVIC_Configuration();
	/* GPIO Configuration */
	GPIO_Configuration();
	set_trace_level(
			0
			| TRACE_LEVEL_DEFAULT
			| TRACE_LEVEL_ADC
			,1);
	dev_uart_add(&dbg_uart);

	/* ADC Configuration */
	ADC_Configuration();

	TRACE(("Application started...\n"));

	/* insert some delay here */

	if (!rcp_init(5)) {
		DECLARE_RCP_ADC(adc1,0,(1<<12)-1, 20);
		DECLARE_RCP_ADC(adc2,0,(1<<12)-1, 20);
		rcp_add(glb.adc1_val, glb.adc2_val, 0, -100.0, 100.0, 0.25, &adc1, &adc2);
	}


	while(1) {
		main_loop();
	}
}
