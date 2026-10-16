#include "stm32f4xx.h"

volatile int16_t enc1, enc2, enc3;

static void delay_ms(uint32_t ms)
{
    SysTick->LOAD = 16000 - 1; SysTick->VAL = 0; SysTick->CTRL = 5;
    while (ms--) { while (!(SysTick->CTRL & (1U << 16))) { } }
}

static void enc_init(TIM_TypeDef *t)
{
    t->SMCR  = 3;                        /* modo encoder, ambos flancos */
    t->CCMR1 = (1U << 0) | (1U << 8);    /* CH1 y CH2 como entrada */
    t->CCER  = 0;
    t->ARR   = 0xFFFF;
    t->CNT   = 0;
    t->CR1   = 1;
}

int main(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN | RCC_APB1ENR_TIM3EN | RCC_APB1ENR_TIM4EN;

    /* Motor1: PA6, PA7 -> TIM3 (AF2) */
    GPIOA->MODER &= ~((3U << 12) | (3U << 14));
    GPIOA->MODER |=  ((2U << 12) | (2U << 14));
    GPIOA->PUPDR |=  ((1U << 12) | (1U << 14));
    GPIOA->AFR[0] |= (2U << 24) | (2U << 28);

    /* Motor2: PB6, PB7 -> TIM4 (AF2) */
    GPIOB->MODER &= ~((3U << 12) | (3U << 14));
    GPIOB->MODER |=  ((2U << 12) | (2U << 14));
    GPIOB->PUPDR |=  ((1U << 12) | (1U << 14));
    GPIOB->AFR[0] |= (2U << 24) | (2U << 28);

    /* Motor3: PA15 -> TIM2_CH1 (AF1), PB3 -> TIM2_CH2 (AF1) */
    GPIOA->MODER &= ~(3U << 30);  GPIOA->MODER |= (2U << 30);
    GPIOA->PUPDR &= ~(3U << 30);  GPIOA->PUPDR |= (1U << 30);
    GPIOA->AFR[1] &= ~(0xFU << 28); GPIOA->AFR[1] |= (1U << 28);
    GPIOB->MODER &= ~(3U << 6);   GPIOB->MODER |= (2U << 6);
    GPIOB->PUPDR &= ~(3U << 6);   GPIOB->PUPDR |= (1U << 6);
    GPIOB->AFR[0] &= ~(0xFU << 12); GPIOB->AFR[0] |= (1U << 12);

    enc_init(TIM3);
    enc_init(TIM4);
    enc_init(TIM2);

    while (1)
    {
        enc1 = (int16_t)TIM3->CNT;
        enc2 = (int16_t)TIM4->CNT;
        enc3 = (int16_t)TIM2->CNT;
        delay_ms(50);
    }
}