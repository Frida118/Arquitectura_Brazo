#include "stm32f4xx.h"

static void delay_ms(uint32_t ms)
{
    SysTick->LOAD = 16000 - 1; SysTick->VAL = 0; SysTick->CTRL = 5;
    while (ms--) { while (!(SysTick->CTRL & (1U << 16))) { } }
}

int main(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN;
    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;

    /* PB10 (nSTBY), PB12 (AIN1), PB13 (AIN2) como salida */
    GPIOB->MODER &= ~((3U << 20) | (3U << 24) | (3U << 26));
    GPIOB->MODER |=  ((1U << 20) | (1U << 24) | (1U << 26));

    /* PA8 = TIM1_CH1 (AF1) */
    GPIOA->MODER &= ~(3U << 16);  GPIOA->MODER |= (2U << 16);
    GPIOA->AFR[1] &= ~0xFU;       GPIOA->AFR[1] |= 1U;

    /* PWM de 1 kHz: 16 MHz / 16 / 1000 */
    TIM1->PSC   = 15;
    TIM1->ARR   = 999;
    TIM1->CCMR1 = (6U << 4) | (1U << 3);
    TIM1->CCER  = TIM_CCER_CC1E;
    TIM1->BDTR |= TIM_BDTR_MOE;
    TIM1->CCR1  = 0;
    TIM1->EGR   = TIM_EGR_UG;
    TIM1->CR1   = TIM_CR1_ARPE | TIM_CR1_CEN;

    GPIOB->BSRR = (1U << 10);                          /* nSTBY = 1 */

    while (1)
    {
        GPIOB->BSRR = (1U << 12) | (1U << (13 + 16));  /* adelante */
        TIM1->CCR1 = 300;  delay_ms(2000);
        TIM1->CCR1 = 0;    delay_ms(1000);

        GPIOB->BSRR = (1U << 13) | (1U << (12 + 16));  /* atrás */
        TIM1->CCR1 = 300;  delay_ms(2000);
        TIM1->CCR1 = 0;    delay_ms(1000);
    }
}