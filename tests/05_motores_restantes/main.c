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

    /* PB0, PB1 (Driver2 AIN1/AIN2), PB10 (nSTBY), PB14, PB15 (Driver1 BIN1/BIN2) */
    GPIOB->MODER &= ~((3U << 0) | (3U << 2) | (3U << 20) | (3U << 28) | (3U << 30));
    GPIOB->MODER |=  ((1U << 0) | (1U << 2) | (1U << 20) | (1U << 28) | (1U << 30));

    /* PA9 = TIM1_CH2, PA10 = TIM1_CH3 (AF1) */
    GPIOA->MODER &= ~((3U << 18) | (3U << 20));
    GPIOA->MODER |=  ((2U << 18) | (2U << 20));
    GPIOA->AFR[1] &= ~((0xFU << 4) | (0xFU << 8));
    GPIOA->AFR[1] |=  ((1U << 4) | (1U << 8));

    TIM1->PSC   = 15;
    TIM1->ARR   = 999;
    TIM1->CCMR1 = (6U << 12) | (1U << 11);   /* CH2 */
    TIM1->CCMR2 = (6U << 4)  | (1U << 3);    /* CH3 */
    TIM1->CCER  = (1U << 4) | (1U << 8);     /* CC2E, CC3E */
    TIM1->BDTR |= TIM_BDTR_MOE;
    TIM1->CCR2  = 0;
    TIM1->CCR3  = 0;
    TIM1->EGR   = TIM_EGR_UG;
    TIM1->CR1   = TIM_CR1_ARPE | TIM_CR1_CEN;

    GPIOB->BSRR = (1U << 10);                /* nSTBY = 1 */

    while (1)
    {
        /* Motor 2: BIN1=1, BIN2=0 */
        GPIOB->BSRR = (1U << 14) | (1U << (15 + 16));
        TIM1->CCR2 = 300;  delay_ms(2000);
        TIM1->CCR2 = 0;    delay_ms(1000);

        /* Motor 3: AIN1=1, AIN2=0 */
        GPIOB->BSRR = (1U << 0) | (1U << (1 + 16));
        TIM1->CCR3 = 300;  delay_ms(2000);
        TIM1->CCR3 = 0;    delay_ms(1000);
    }
}