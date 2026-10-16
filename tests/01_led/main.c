/* Prueba 01: LED externo en PB5
 * Objetivo: comprobar GPIO de salida y cableado del LED (R1 330 ohm a PB5).
 * Reloj: HSI por defecto (16 MHz), sin configurar PLL.
 * Resultado esperado: el LED parpadea aprox. 1 vez por segundo (no medido).
 */
#include "stm32f4xx.h"

static void espera(volatile uint32_t n)
{
    while (n--) { }
}

int main(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;   /* reloj del puerto B */

    GPIOB->MODER &= ~(3U << (5 * 2));      /* limpia modo de PB5 */
    GPIOB->MODER |=  (1U << (5 * 2));      /* PB5 como salida */

    while (1)
    {
        GPIOB->ODR ^= (1U << 5);           /* cambia el estado del LED */
        espera(1000000);                   /* retardo aproximado, ajustar si es necesario */
    }
}
