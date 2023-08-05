#include "main.h"

#define DELAY_VAL 1000000
char RxBuffer[256];
char TxBuffer[256];
bool CommandRecieved = false;
bool button[9];
bool column[3];
bool send = 0;
uint8_t num = 0;


void init_gpio(void)
{
	// PC0-PC2 Pull down input, PC3-PC5 output 50 MHz
	RCC->APB2ENR |= RCC_APB2ENR_IOPCEN | RCC_APB2ENR_AFIOEN; //тактирование порта С и альтернативной функции потока ввода/вывода
	GPIOC->CRL &= ~(GPIO_CRL_CNF3|GPIO_CRL_CNF4|GPIO_CRL_CNF5); // pin3-5 push-pull ("00")
	GPIOC->CRL |= GPIO_CRL_MODE3|GPIO_CRL_MODE4|GPIO_CRL_MODE5; // pin3-5 Output mode 50mHz ("11")

	GPIOC->CRL |= GPIO_CRL_CNF0_1|GPIO_CRL_CNF1_1|GPIO_CRL_CNF2_1;
	GPIOC->CRL &= ~(GPIO_CRL_CNF0_0|GPIO_CRL_MODE0|GPIO_CRL_CNF1_0|GPIO_CRL_MODE1|GPIO_CRL_CNF2_0|GPIO_CRL_MODE2); //pin0-2 pull down input

	GPIOC->BSRR = GPIO_BSRR_BR0|GPIO_BSRR_BR1|GPIO_BSRR_BR2;

}

void delay(uint32_t delay_value)
{
	for(uint32_t i = 0; i< delay_value; i++);
}

void init_interrupt()
{
	EXTI->IMR |= EXTI_IMR_MR0|EXTI_IMR_MR1|EXTI_IMR_MR2; //накладываем маску и разрешаем прерывания
	EXTI->RTSR |= EXTI_RTSR_TR0|EXTI_RTSR_TR1|EXTI_RTSR_TR2; //по возрастающему фронту реагирует

	AFIO->EXTICR[0] |= AFIO_EXTICR1_EXTI0_PC|AFIO_EXTICR1_EXTI1_PC|AFIO_EXTICR1_EXTI2_PC; //включение альтернативной функции потока ввода/вывода пинов

	NVIC_EnableIRQ(EXTI0_IRQn);  //разрешаем прерывания на линии 0
	NVIC_SetPriority(EXTI0_IRQn,0); //приоритет прерывания

	NVIC_EnableIRQ(EXTI1_IRQn);  //разрешаем прерывания на линии 0
	NVIC_SetPriority(EXTI1_IRQn,0); //приоритет прерывания

	NVIC_EnableIRQ(EXTI2_IRQn);  //разрешаем прерывания на линии 0
	NVIC_SetPriority(EXTI2_IRQn,0); //приоритет прерывания

}


void EXTI0_IRQHandler(void)  //вызывается при нажатии на кнопку 1 строки
{
	if(EXTI->PR & EXTI_PR_PR0) //проверка, что прерывание по 0 линии
	{
		if(column[0] == 1)
		{
			button[0] = 1;
		}
		if(column[1] == 1)
		{
			button[1] = 1;
		}
		if(column[2] == 1)
		{
			button[2] = 1;
		}
		send = 1;
		delay(DELAY_VAL);
		EXTI->PR |= EXTI_PR_PR0;
	}
}

void EXTI1_IRQHandler(void)  //вызывается при нажатии на кнопку 2 строки
{
	if(EXTI->PR & EXTI_PR_PR1) //проверка, что прерывание по 1 линии
	{
		if(column[0] == 1)
		{
			button[3] = 1;
		}
		if(column[1] == 1)
		{
			button[4] = 1;
		}
		if(column[2] == 1)
		{
			button[5] = 1;
		}
		send = 1;
		delay(DELAY_VAL);
		EXTI->PR |= EXTI_PR_PR1;
	}
}

void EXTI2_IRQHandler(void)  //вызывается при нажатии на кнопку 3 строки
{
	if(EXTI->PR & EXTI_PR_PR2) //проверка, что прерывание по 2 линии
	{
		if(column[0] == 1)
		{
			button[6] = 1;
		}
		if(column[1] == 1)
		{
			button[7] = 1;
		}
		if(column[2] == 1)
		{
			button[8] = 1;
		}
		send = 1;
		delay(DELAY_VAL);
		EXTI->PR |= EXTI_PR_PR2;
	}

}

void initClk(void)
{
	// Enable HSI
	RCC->CR |= RCC_CR_HSION;
	while(!(RCC->CR & RCC_CR_HSIRDY)){};

	// Enable Prefetch Buffer
	FLASH->ACR |= FLASH_ACR_PRFTBE;

	// Flash 2 wait state
	FLASH->ACR &= ~FLASH_ACR_LATENCY;
	FLASH->ACR |= FLASH_ACR_LATENCY_2;

	// HCLK = SYSCLK
	RCC->CFGR |= RCC_CFGR_HPRE_DIV1;

	// PCLK2 = HCLK
	RCC->CFGR |= RCC_CFGR_PPRE2_DIV1;

	// PCLK1 = HCLK
	RCC->CFGR |= RCC_CFGR_PPRE1_DIV2;

	// PLL configuration: PLLCLK = HSI/2 * 16 = 64 MHz
	RCC->CFGR &= ~RCC_CFGR_PLLSRC;
	RCC->CFGR |= RCC_CFGR_PLLMULL16;

	// Enable PLL
	RCC->CR |= RCC_CR_PLLON;

	// Wait till PLL is ready
	while((RCC->CR & RCC_CR_PLLRDY) == 0) {};

	// Select PLL as system clock source
	RCC->CFGR &= ~RCC_CFGR_SW;
	RCC->CFGR |= RCC_CFGR_SW_PLL;

	// Wait till PLL is used as system clock source
	while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL){};
}

void init_UART2(void)
{
	RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
	RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;

	//PA2 - OUT, PA3 - IN
	GPIOA->CRL &= ~ (GPIO_CRL_CNF2 | GPIO_CRL_MODE2);
	GPIOA->CRL |= GPIO_CRL_CNF2_1| GPIO_CRL_MODE2_1; //PA2 AFO push-pull, output 2MHz

	GPIOA->CRL &= ~ (GPIO_CRL_CNF3 | GPIO_CRL_MODE3);
	GPIOA->CRL |= GPIO_CRL_CNF3_0; //PA3 floating input

	//частота APB1 (f(APB2)/2) /(16*115200) = 17.4 |||| 17 в hex = 0x11; 0.4*16=6.4 в hex в итоге = 0x116
	USART2->BRR = 0x116; //115200 бод
	USART2->CR1 |= USART_CR1_UE|USART_CR1_TE; //вкл uart, вкл Tx
}

void TxStr(char *str, bool crlf)
{
	if (crlf)
	{
		strcat(str, "\r"); //конкатинация символов конца строки
	}
	for(uint16_t i = 0; i<strlen(str); i++)
	{
		USART2->DR = str[i]; //побайтово отправляем
		while((USART2->SR & USART_SR_TC) == 0) {}; //ждем подтверждения что байт передан
	}
}

uint8_t position(void)
{
	for(uint8_t i = 0; i<8; i++)
	{
		if (button[i] == 1)
		{
			memset(button,0,9);
			return i;
		}
	}
}

void ExecuteCommand(void)
{
	memset(TxBuffer,0,256);

	if(send)
	{
		num = position();
		num++;
		sprintf(TxBuffer, "%d", num);
	}
	TxStr(TxBuffer, false);
	send = 0;
}

int main(void)
{
	init_gpio();
	initClk();
	init_interrupt();
	init_UART2();

	memset(button,0,9);
	memset(column,0,3);
	GPIOC->BSRR = GPIO_BSRR_BR3|GPIO_BSRR_BR4|GPIO_BSRR_BR5;

    while(true)
    {
    	if(send)
		{
			ExecuteCommand();
		}
    	column[0] = 1;
    	GPIOC->BSRR = GPIO_BSRR_BS3;
    	delay(100000);
    	GPIOC->BSRR = GPIO_BSRR_BR3;
    	column[0] = 0;

    	column[1] = 1;
    	GPIOC->BSRR = GPIO_BSRR_BS4;
    	delay(100000);
    	GPIOC->BSRR = GPIO_BSRR_BR4;
    	column[1] = 0;

    	column[2] = 1;
    	GPIOC->BSRR = GPIO_BSRR_BS5;
    	delay(100000);
		GPIOC->BSRR = GPIO_BSRR_BR5;
		column[2] = 0;

    }

}
