#include "stm32f4xx.h"

volatile uint16_t valor;
volatile uint16_t minimo = 4095;
volatile uint16_t maximo = 0;

static void delay_ms(uint32_t ms)
{
    SysTick->LOAD = 16000 - 1; SysTick->VAL = 0; SysTick->CTRL = 5;
    while (ms--) { while (!(SysTick->CTRL & (1U << 16))) { } }
}

int main(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;

    GPIOA->MODER |= (3U << 4);          /* PA2 analógico */

    ADC1->SMPR2 = (7U << 6);            /* canal 2, muestreo largo */
    ADC1->SQR1  = 0;
    ADC1->SQR3  = 2;                    /* canal 2 */
    ADC1->CR2   = ADC_CR2_ADON;
    delay_ms(1);

    while (1)
    {
        ADC1->CR2 |= ADC_CR2_SWSTART;
        while (!(ADC1->SR & ADC_SR_EOC)) { }
        valor = (uint16_t)ADC1->DR;
        if (valor < minimo) minimo = valor;
        if (valor > maximo) maximo = valor;
        delay_ms(5);
    }
}