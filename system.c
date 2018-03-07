/*
*
* Copyright (C) Patryk Jaworski <regalis@regalis.com.pl>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*
*/
#include <stm32l476xx.h>

/* Helpers for SystemInitError() */
#define SYSTEM_INIT_ERROR_FLASH 0x01
#define SYSTEM_INIT_ERROR_PLL 0x02
#define SYSTEM_INIT_ERROR_CLKSRC 0x04
#define SYSTEM_INIT_ERROR_HSI 0x08

void SystemInitError(uint8_t error_source) {
  while(1);
}

void SystemInit() {
  RCC->CR |= (1<<8);
  /* Wait until HSI ready */
  while ((RCC->CR & RCC_CR_HSIRDY) == 0);

  /* Disable main PLL */
  RCC->CR &= ~(RCC_CR_PLLON);
  /* Wait until PLL ready (disabled) */
  while ((RCC->CR & RCC_CR_PLLRDY) != 0);

  /* Configure Main PLL */
  RCC->PLLCFGR = (1<<24)|(10<<8)|2;

  /* PLL On */
  RCC->CR |= RCC_CR_PLLON;
  /* Wait until PLL is locked */
  while ((RCC->CR & RCC_CR_PLLRDY) == 0);

  /*
   * FLASH configuration block
   * enable instruction cache
   * enable prefetch
   * set latency to 2WS (3 CPU cycles)
   */
  FLASH->ACR |= FLASH_ACR_ICEN | FLASH_ACR_PRFTEN | FLASH_ACR_LATENCY_2WS;

  /* Check flash latency */
  if ((FLASH->ACR & FLASH_ACR_LATENCY) != FLASH_ACR_LATENCY_2WS) {
    SystemInitError(SYSTEM_INIT_ERROR_FLASH);
  }

  /* Set clock source to PLL */
  RCC->CFGR |= RCC_CFGR_SW_PLL;
  /* Check clock source */
  while ((RCC->CFGR & RCC_CFGR_SWS_PLL) != RCC_CFGR_SWS_PLL);

  // Enable GPIOA clock
  RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
  // A2 -> USART2_TX
  GPIOA->AFR[0] &= ~GPIO_AFRL_AFSEL2_Msk;
  GPIOA->AFR[0] |= GPIO_AFRL_AFSEL2_0 | GPIO_AFRL_AFSEL2_1 | GPIO_AFRL_AFSEL2_2;
  // A8,A9,A10 -> TIM1
  GPIOA->AFR[1] &= ~(GPIO_AFRH_AFSEL8_Msk|GPIO_AFRH_AFSEL9_Msk|GPIO_AFRH_AFSEL10_Msk);
  GPIOA->AFR[1] |= GPIO_AFRH_AFSEL8_0 | GPIO_AFRH_AFSEL9_0 | GPIO_AFRH_AFSEL10_0;
  // PORTA Modes
  GPIOA->MODER = 0xABEAFFEF;
  //GPIOA->MODER = 0xABFFFFEF;

  // Enable USART2 clock
  RCC->APB1ENR1 |= RCC_APB1ENR1_USART2EN;
  // Disable USART2
  USART2->CR1 = 0;
  // Set data rate
  USART2->BRR = 694; //115200
  // Enable USART2
  USART2->CR1 |= USART_CR1_UE;
  // Enable transmit
  USART2->CR1 |= USART_CR1_TE;

  // TIM1
  RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;
  TIM1->CR1 &= ~1;
  TIM1->CCR1 = 20000;
  TIM1->CCR2 = 20000;
  TIM1->CCR3 = 20000;
  TIM1->CCMR1 = (6<<12)|(6<<4);
  TIM1->CCMR2 = (6<<4);
  TIM1->CCER = (1<<0)|(1<<4)|(1<<8);
  TIM1->BDTR = (1<<15);
  TIM1->CR1 |= 1;
}
