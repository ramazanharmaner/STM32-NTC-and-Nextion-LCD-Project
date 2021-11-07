
#include "stm32f10x.h"
#include <stdio.h>
#include <math.h>


void delay(unsigned int nCount);
/* User defined function prototypes */
void GPIOC_Init(void);
void USART1_Init(void);
void led_toggle(void);
void ADC_Configuration(void);
u16 readADC1(u8 channel);
void putch(char c);
void putst(char *str);

char out_buffer[50];

#define TK_0 273.15
#define TK_25 298.15
#define Beta 3892.0
#define RNtc_25 2252.0
#define VSupply 3.3
#define RConstant 4400.0

u16 adcValue;
float VNtc,RNtc,TNtc;



int main(void) {
	/*!< At this stage the microcontroller clock setting is already configured,
	 this is done through SystemInit() function which is called from startup
	 file (startup_stm32f0xx.s) before to branch to application main.
	 To reconfigure the default setting of SystemInit() function, refer to
	 system_stm32f0xx.c file
	 */

	/* Initialize GPIOA PIN8 */
	GPIOC_Init();
	/* Initialize USART1 */
	USART1_Init();

	ADC_Configuration();

	sprintf(out_buffer, "Hello world\r\n");
	putst(out_buffer);

	while (1) {
		adcValue = readADC1(ADC_Channel_8);
		VNtc = (float)(adcValue*VSupply/4096);
		RNtc = RConstant*VNtc/(VSupply-VNtc);
		TNtc = logf(RNtc/RNtc_25)/Beta + 1/ TK_25;
		TNtc = 1/TNtc - TK_0;


		sprintf(out_buffer, "%f\r\n",TNtc);
		putst(out_buffer);
		led_toggle();
		delay(150);

		/* Do nothing, all happens in ISR */

	}
}

/***********************************************
 * Initialize GPIOC PIN13 as push-pull output
 ***********************************************/
void GPIOC_Init(void) {

	GPIO_InitTypeDef gpioc_init_struct;

	/* Enable PORT C clock */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

	gpioc_init_struct.GPIO_Mode = GPIO_Mode_Out_PP;

	gpioc_init_struct.GPIO_Pin = GPIO_Pin_13;

	gpioc_init_struct.GPIO_Speed = GPIO_Speed_50MHz;
	/* Initialize GPIOA: 50MHz, PIN13, Push-pull Output */
	GPIO_Init(GPIOC, &gpioc_init_struct);

	/* Turn off LED to start with */
	GPIO_SetBits(GPIOC, GPIO_Pin_13);
}

/*****************************************************
 * Initialize USART1: enable interrupt on reception
 * of a character
 *****************************************************/
void USART1_Init(void) {
	/* USART configuration structure for USART1 */
	USART_InitTypeDef usart1_init_struct;
	/* Bit configuration structure for GPIOA PIN9 and PIN10 */
	GPIO_InitTypeDef gpioa_init_struct;

	/* Enalbe clock for USART1, AFIO and GPIOA */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_AFIO |
	RCC_APB2Periph_GPIOA, ENABLE);

	/* GPIOA PIN9 alternative function Tx */
	gpioa_init_struct.GPIO_Pin = GPIO_Pin_9;
	gpioa_init_struct.GPIO_Speed = GPIO_Speed_50MHz;
	gpioa_init_struct.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &gpioa_init_struct);
	/* GPIOA PIN9 alternative function Rx */
	gpioa_init_struct.GPIO_Pin = GPIO_Pin_10;
	gpioa_init_struct.GPIO_Speed = GPIO_Speed_50MHz;
	gpioa_init_struct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &gpioa_init_struct);

	/* Enable USART1 */
	USART_Cmd(USART1, ENABLE);
	/* Baud rate 9600, 8-bit data, One stop bit
	 * No parity, Do both Rx and Tx, No HW flow control
	 */
	usart1_init_struct.USART_BaudRate = 19200;
	usart1_init_struct.USART_WordLength = USART_WordLength_8b;
	usart1_init_struct.USART_StopBits = USART_StopBits_1;
	usart1_init_struct.USART_Parity = USART_Parity_No;
	usart1_init_struct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	usart1_init_struct.USART_HardwareFlowControl =
	USART_HardwareFlowControl_None;
	/* Configure USART1 */
	USART_Init(USART1, &usart1_init_struct);
	/* Enable RXNE interrupt */
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
	/* Enable USART1 global interrupt */
	NVIC_EnableIRQ(USART1_IRQn);
}

/*******************************************
 * Toggle LED
 *******************************************/
void led_toggle(void) {
	/* Read LED output (GPIOC PIN13) status */
	uint8_t led_bit = GPIO_ReadOutputDataBit(GPIOC, GPIO_Pin_13);

	/* If LED output set, clear it */
	if (led_bit == (uint8_t) Bit_SET) {
		GPIO_ResetBits(GPIOC, GPIO_Pin_13);
	}
	/* If LED output clear, set it */
	else {
		GPIO_SetBits(GPIOC, GPIO_Pin_13);
	}
}

/**********************************************************
 * USART1 interrupt request handler: on reception of a
 * character 't', toggle LED and transmit a character 'T'
 *********************************************************/
void USART1_IRQHandler(void) {
	/* RXNE handler */
	if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
		/* If received 't', toggle LED and transmit 'T' */
		if ((char) USART_ReceiveData(USART1) == 't') {
			led_toggle();
			USART_SendData(USART1, 'T');
			/* Wait until Tx data register is empty, not really
			 * required for this example but put in here anyway.
			 */
			/*
			 while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET)
			 {
			 }*/
		}
	}

	/* ------------------------------------------------------------ */
	/* Other USART1 interrupts handler can go here ...             */
}

void ADC_Configuration(void) {

	GPIO_InitTypeDef gpiob_init_struct;

	ADC_InitTypeDef ADC_InitStructure;
	/* PCLK2 is the APB2 clock */
	/* ADCCLK = PCLK2/6 = 72/6 = 12MHz*/
	RCC_ADCCLKConfig(RCC_PCLK2_Div6);

	/* Enable ADC1 clock so that we can talk to it */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
	/* Put everything back to power-on defaults */
	ADC_DeInit(ADC1);

	// C - Init the GPIO with the structure - Testing ADC
	gpiob_init_struct.GPIO_Pin = GPIO_Pin_0;
	gpiob_init_struct.GPIO_Speed = GPIO_Speed_2MHz;
	gpiob_init_struct.GPIO_Mode = GPIO_Mode_IN_FLOATING;

	/* Initialize GPIOA: 50MHz, PIN13, Push-pull Output */
	GPIO_Init(GPIOB, &gpiob_init_struct);

	/* ADC1 Configuration ------------------------------------------------------*/
	/* ADC1 and ADC2 operate independently */
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
	/* Disable the scan conversion so we do one at a time */
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;
	/* Don't do contimuous conversions - do them on demand */
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
	/* Start conversin by software, not an external trigger */
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	/* Conversions are 12 bit - put them in the lower 12 bits of the result */
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	/* Say how many channels would be used by the sequencer */
	ADC_InitStructure.ADC_NbrOfChannel = 1;

	/* Now do the setup */
	ADC_Init(ADC1, &ADC_InitStructure);
	/* Enable ADC1 */
	ADC_Cmd(ADC1, ENABLE);

	/* Enable ADC1 reset calibaration register */
	ADC_ResetCalibration(ADC1);
	/* Check the end of ADC1 reset calibration register */
	while (ADC_GetResetCalibrationStatus(ADC1))
		;
	/* Start ADC1 calibaration */
	ADC_StartCalibration(ADC1);
	/* Check the end of ADC1 calibration */
	while (ADC_GetCalibrationStatus(ADC1))
		;
}

u16 readADC1(u8 channel) {
	ADC_RegularChannelConfig(ADC1, channel, 1, ADC_SampleTime_7Cycles5);
	// Start the conversion
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);
	// Wait until conversion completion
	while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET)
		;
	// Get the conversion value
	return ADC_GetConversionValue(ADC1);
}

//writes a character to the serial port
void putch(char c) {
	USART_SendData(USART1, c);
	while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET)
		;
}

// writes a NULL terminated string to serial port
void putst(char *str) {
	while (*str != 0) {

		putch(*str);
		str++;
	}
}

#ifdef  USE_FULL_ASSERT

/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t* file, uint32_t line)
{
	/* User can add his own implementation to report the file name and line number,
	 ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

	/* Infinite loop */
	while (1)
	{
	}
}
#endif

void delay(unsigned int nCount) {
	unsigned int i, j;

	for (i = 0; i < nCount; i++)
		for (j = 0; j < 0x2AFF; j++)
			;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
