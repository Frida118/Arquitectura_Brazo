#include "stm32f4xx.h"

#define UMBRAL 2048U            /* PROVISIONAL: ajustar con tus lecturas */
volatile uint16_t valor;

static void delay_ms(uint32_t ms)
{
    SysTick->LOAD = 16000 - 1; SysTick->VAL = 0; SysTick->CTRL = 5;
    while (ms--) { while (!(SysTick->CTRL & (1U << 16))) { } }
}

int main(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN;
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;

    GPIOB->MODER &= ~(3U << 10);  GPIOB->MODER |= (1U << 10);   /* PB5 salida */
    GPIOA->MODER |= (3U << 8);                                  /* PA4 analógico */

    ADC1->SMPR2 = (7U << 12);     /* canal 4, muestreo largo */
    ADC1->SQR1  = 0;              /* 1 conversión */
    ADC1->SQR3  = 4;              /* canal 4 */
    ADC1->CR2   = ADC_CR2_ADON;
    delay_ms(1);

    while (1)
    {
        ADC1->CR2 |= ADC_CR2_SWSTART;
        while (!(ADC1->SR & ADC_SR_EOC)) { }
        valor = (uint16_t)ADC1->DR;

        if (valor > UMBRAL) GPIOB->BSRR = (1U << 5);
        else                GPIOB->BSRR = (1U << (5 + 16));
        delay_ms(50);
    }
}