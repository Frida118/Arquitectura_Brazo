#include "stm32f4xx.h"

volatile uint8_t encontrados[8];
volatile uint8_t cuantos;

static void delay_ms(uint32_t ms)
{
    SysTick->LOAD = 16000 - 1; SysTick->VAL = 0; SysTick->CTRL = 5;
    while (ms--) { while (!(SysTick->CTRL & (1U << 16))) { } }
}

static uint8_t probar(uint8_t dir)
{
    uint32_t t;
    uint8_t ok = 0;

    I2C1->CR1 |= I2C_CR1_START;
    t = 20000;
    while (!(I2C1->SR1 & I2C_SR1_SB)) { if (--t == 0) break; }

    I2C1->DR = (uint8_t)(dir << 1);       /* escritura */
    t = 20000;
    while (!(I2C1->SR1 & (I2C_SR1_ADDR | I2C_SR1_AF))) { if (--t == 0) break; }

    if (I2C1->SR1 & I2C_SR1_ADDR) {
        ok = 1;
        (void)I2C1->SR1; (void)I2C1->SR2; /* limpia ADDR */
    }
    I2C1->SR1 &= ~I2C_SR1_AF;
    I2C1->CR1 |= I2C_CR1_STOP;
    delay_ms(1);
    return ok;
}

int main(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;

    GPIOB->MODER &= ~(3U << 10);  GPIOB->MODER |= (1U << 10);   /* PB5 LED */

    /* PB8 SCL, PB9 SDA: AF4, open-drain */
    GPIOB->MODER &= ~((3U << 16) | (3U << 18));
    GPIOB->MODER |=  ((2U << 16) | (2U << 18));
    GPIOB->OTYPER |= (1U << 8) | (1U << 9);
    GPIOB->OSPEEDR |= (3U << 16) | (3U << 18);
    GPIOB->AFR[1] &= ~0xFFU;
    GPIOB->AFR[1] |= (4U << 0) | (4U << 4);

    /* 100 kHz con APB1 = 16 MHz */
    I2C1->CR1   = 0;
    I2C1->CR2   = 16;
    I2C1->CCR   = 80;
    I2C1->TRISE = 17;
    I2C1->CR1  |= I2C_CR1_PE;

    cuantos = 0;
    for (uint8_t d = 0x08; d < 0x78; d++) {
        if (probar(d) && cuantos < 8) encontrados[cuantos++] = d;
    }

    while (1)
    {
        for (uint8_t i = 0; i < cuantos; i++) {
            GPIOB->BSRR = (1U << 5);        delay_ms(200);
            GPIOB->BSRR = (1U << (5 + 16)); delay_ms(200);
        }
        delay_ms(1500);
    }
}