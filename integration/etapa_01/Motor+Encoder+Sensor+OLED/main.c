/* ============================================================
 *  Motor DC con Encoder + Sensor Flexión + OLED SSD1306
 *  STM32F401CCUx (Black Pill, Cortex-M4)
 *
 *  Sensor flexión -> PA4 (ADC1_IN4)
 *  Encoder C1     -> PA0 (TIM2_CH1)
 *  Encoder C2     -> PA1 (TIM2_CH2)
 *  PWM motor      -> PA8 (TIM1_CH1)
 *  AIN1           -> PB12
 *  AIN2           -> PB13
 *  nSTBY          -> PB10
 *  OLED SCL       -> PB8  (I2C1_SCL)
 *  OLED SDA       -> PB9  (I2C1_SDA)
 *
 *  COMPORTAMIENTO:
 *  - Sin flexión  -> motor detenido, OLED: "MOTOR REPOSO"
 *  - Con flexión  -> motor gira velocidad fija, OLED: "MOTOR GIRANDO"
 * ============================================================ */

#include <stdint.h>
#include "stm32f4xx.h"

/* ------------------- DEFINICIONES ------------------------- */
#define SYSCLK_HZ           84000000UL

#define PWM_PSC             83U
#define PWM_ARR             999U

#define FLEX_THRESHOLD      700U
#define MOTOR_SPEED         500U
#define OLED_ADDR           0x3CU

/* ------------------- VARIABLES GLOBALES ------------------- */
volatile uint32_t systick_ms = 0;

/* ------------------- SYSTICK ------------------------------ */
void SysTick_Handler(void)
{
    systick_ms++;
}

static void SysTick_Init(void)
{
    SysTick->LOAD = (SYSCLK_HZ / 1000UL) - 1UL;
    SysTick->VAL  = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |
                    SysTick_CTRL_TICKINT_Msk    |
                    SysTick_CTRL_ENABLE_Msk;
}

static void delay_ms(uint32_t ms)
{
    uint32_t start = systick_ms;
    while ((systick_ms - start) < ms) { }
}

/* ------------------- RELOJ PLL F401 ----------------------- */
static void Clock_Init(void)
{
    RCC->CR |= RCC_CR_HSION;
    while (!(RCC->CR & RCC_CR_HSIRDY)) { }

    RCC->PLLCFGR = (16U  <<  0) |
                   (336U <<  6) |
                   (1U   << 16) |
                   (0U   << 22) |
                   (7U   << 24);

    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY)) { }

    FLASH->ACR = FLASH_ACR_LATENCY_2WS |
                 FLASH_ACR_PRFTEN      |
                 FLASH_ACR_ICEN        |
                 FLASH_ACR_DCEN;

    RCC->CFGR = RCC_CFGR_PPRE1_DIV2;

    RCC->CFGR |= RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) { }
}

/* ------------------- GPIO --------------------------------- */
static void GPIO_Init_All(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN |
                    RCC_AHB1ENR_GPIOBEN;

    /* PA0, PA1: AF1 (TIM2_CH1, TIM2_CH2) encoder */
    GPIOA->MODER  &= ~( (3U<<(0*2))|(3U<<(1*2)) );
    GPIOA->MODER  |=  ( (2U<<(0*2))|(2U<<(1*2)) );
    GPIOA->PUPDR  &= ~( (3U<<(0*2))|(3U<<(1*2)) );
    GPIOA->PUPDR  |=  ( (1U<<(0*2))|(1U<<(1*2)) );
    GPIOA->AFR[0] &= ~( (0xFU<<(0*4))|(0xFU<<(1*4)) );
    GPIOA->AFR[0] |=  ( (1U<<(0*4))|(1U<<(1*4)) );

    /* PA4: analógico (ADC1_IN4) */
    GPIOA->MODER  |=  (3U<<(4*2));
    GPIOA->PUPDR  &= ~(3U<<(4*2));

    /* PA8: AF1 (TIM1_CH1) PWM motor */
    GPIOA->MODER  &= ~(3U<<(8*2));
    GPIOA->MODER  |=  (2U<<(8*2));
    GPIOA->AFR[1] &= ~(0xFU<<0);
    GPIOA->AFR[1] |=  (1U<<0);

    /* PB8: I2C1_SCL, PB9: I2C1_SDA — AF4, open-drain */
    GPIOB->MODER  &= ~( (3U<<(8*2))|(3U<<(9*2)) );
    GPIOB->MODER  |=  ( (2U<<(8*2))|(2U<<(9*2)) );
    GPIOB->OTYPER |=  ( (1U<<8)|(1U<<9) );
    GPIOB->OSPEEDR|=  ( (3U<<(8*2))|(3U<<(9*2)) );
    GPIOB->PUPDR  &= ~( (3U<<(8*2))|(3U<<(9*2)) );
    GPIOB->PUPDR  |=  ( (1U<<(8*2))|(1U<<(9*2)) );
    GPIOB->AFR[1] &= ~( (0xFU<<(0*4))|(0xFU<<(1*4)) );
    GPIOB->AFR[1] |=  ( (4U<<(0*4))|(4U<<(1*4)) );

    /* PB10: nSTBY TB6612FNG */
    GPIOB->MODER &= ~(3U<<(10*2));
    GPIOB->MODER |=  (1U<<(10*2));
    GPIOB->ODR   &= ~(1U<<10);

  /* PB12: AIN1, PB13: AIN2 */
    GPIOB->MODER &= ~( (3U<<(12*2))|(3U<<(13*2)) );
    GPIOB->MODER |=  ( (1U<<(12*2))|(1U<<(13*2)) );
    GPIOB->ODR   &= ~( (1U<<12)|(1U<<13) );
}

/* ------------------- ADC1 --------------------------------- */
static void ADC_Init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;

    ADC1->CR2   = 0;
    ADC1->CR1   = 0;
    ADC1->SMPR2 = (7U << (4*3));
    ADC1->SQR1  = 0;
    ADC1->SQR3  = 4U;

    ADC1->CR2 |= ADC_CR2_ADON;
    delay_ms(1);
}

static uint16_t ADC_Read(void)
{
    ADC1->CR2 |= ADC_CR2_SWSTART;
    while (!(ADC1->SR & ADC_SR_EOC)) { }
    return (uint16_t)(ADC1->DR);
}

/* ------------------- TIM2 ENCODER ------------------------- */
static void TIM2_Encoder_Init(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    TIM2->CR1   = 0;
    TIM2->SMCR  = TIM_SMCR_SMS_0 | TIM_SMCR_SMS_1;
    TIM2->CCMR1 = (1U<<TIM_CCMR1_CC1S_Pos) |
                  (1U<<TIM_CCMR1_CC2S_Pos);
    TIM2->CCER  = 0;
    TIM2->ARR   = 0xFFFFFFFFU;
    TIM2->CNT   = 0;
    TIM2->EGR   = TIM_EGR_UG;
    TIM2->CR1  |= TIM_CR1_CEN;
}

static int32_t Encoder_Read_Reset(void)
{
    int32_t count = (int32_t)TIM2->CNT;
    TIM2->CNT = 0;
    return count;
}

/* ------------------- TIM1 PWM ----------------------------- */
static void TIM1_PWM_Init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;

    TIM1->PSC = PWM_PSC;
    TIM1->ARR = PWM_ARR;

    TIM1->CCMR1 &= ~TIM_CCMR1_OC1M;
    TIM1->CCMR1 |=  (6U<<TIM_CCMR1_OC1M_Pos) | TIM_CCMR1_OC1PE;
    TIM1->CCER  |=  TIM_CCER_CC1E;
    TIM1->BDTR  |=  TIM_BDTR_MOE;
    TIM1->CCR1   =  0;
    TIM1->CR1   |=  TIM_CR1_ARPE;
    TIM1->EGR    =  TIM_EGR_UG;
    TIM1->CR1   |=  TIM_CR1_CEN;
}

/* ------------------- MOTOR -------------------------------- */
static void Motor_Enable(void)  { GPIOB->ODR |=  (1U<<10); }
static void Motor_Disable(void) { GPIOB->ODR &= ~(1U<<10); }

static void Motor_Set(uint32_t duty, uint8_t dir)
{
    if (duty > PWM_ARR) duty = PWM_ARR;
    if (dir) {
        GPIOB->ODR |=  (1U<<12);
        GPIOB->ODR &= ~(1U<<13);
    } else {
        GPIOB->ODR &= ~(1U<<12);
        GPIOB->ODR |=  (1U<<13);
    }
    TIM1->CCR1 = duty;
}

static void Motor_Stop(void)
{
    GPIOB->ODR &= ~((1U<<12)|(1U<<13));
    TIM1->CCR1  = 0;
}

/* ------------------- I2C1 --------------------------------- */
static void I2C1_Init(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;

    I2C1->CR1 = 0;

    /*
     * APB1 = 42MHz
     * Standard mode 100kHz:
     *   FREQ  = 42
     *   CCR   = 42MHz / (2 * 100kHz) = 210
     *   TRISE = 42 + 1 = 43
     */
    I2C1->CR2   = 42U;
    I2C1->CCR   = 210U;
    I2C1->TRISE = 43U;
    I2C1->CR1  |= I2C_CR1_PE;
}

static uint8_t I2C1_Start(uint8_t addr, uint8_t rw)
{
    uint32_t t;

    I2C1->CR1 |= I2C_CR1_START;
    t = systick_ms;
    while (!(I2C1->SR1 & I2C_SR1_SB)) {
        if ((systick_ms - t) > 5U) return 0;
    }

    I2C1->DR = (uint8_t)((addr << 1) | rw);
    t = systick_ms;
    while (!(I2C1->SR1 & I2C_SR1_ADDR)) {
        if (I2C1->SR1 & I2C_SR1_AF) {
            I2C1->SR1 &= ~I2C_SR1_AF;
            I2C1->CR1 |= I2C_CR1_STOP;
            return 0;
        }
        if ((systick_ms - t) > 5U) return 0;
    }
    (void)I2C1->SR1;
    (void)I2C1->SR2;
    return 1;
}

static void I2C1_Write_Byte(uint8_t data)
{
    uint32_t t = systick_ms;
    while (!(I2C1->SR1 & I2C_SR1_TXE)) {
        if ((systick_ms - t) > 5U) return;
    }
    I2C1->DR = data;
}

static void I2C1_Stop(void)
{
    uint32_t t = systick_ms;
    while (!(I2C1->SR1 & I2C_SR1_BTF)) {
        if ((systick_ms - t) > 5U) break;
    }
    I2C1->CR1 |= I2C_CR1_STOP;
}

static void I2C1_Write(uint8_t addr, uint8_t *data, uint8_t len)
{
    if (!I2C1_Start(addr, 0)) return;
    for (uint8_t i = 0; i < len; i++) {
        I2C1_Write_Byte(data[i]);
    }
    I2C1_Stop();
}

/* ------------------- OLED SSD1306 ------------------------- */
static void OLED_Cmd(uint8_t cmd)
{
    uint8_t buf[2] = {0x00, cmd};
    I2C1_Write(OLED_ADDR, buf, 2);
}

static void OLED_Init(void)
{
    delay_ms(100);
    OLED_Cmd(0xAE);
    OLED_Cmd(0x20); OLED_Cmd(0x00);
    OLED_Cmd(0xB0);
    OLED_Cmd(0xC8);
    OLED_Cmd(0x00);
    OLED_Cmd(0x10);
    OLED_Cmd(0x40);
    OLED_Cmd(0x81); OLED_Cmd(0xFF);
    OLED_Cmd(0xA1);
    OLED_Cmd(0xA6);
    OLED_Cmd(0xA8); OLED_Cmd(0x3F);
    OLED_Cmd(0xA4);
    OLED_Cmd(0xD3); OLED_Cmd(0x00);
    OLED_Cmd(0xD5); OLED_Cmd(0xF0);
    OLED_Cmd(0xD9); OLED_Cmd(0x22);
    OLED_Cmd(0xDA); OLED_Cmd(0x12);
    OLED_Cmd(0xDB); OLED_Cmd(0x20);
    OLED_Cmd(0x8D); OLED_Cmd(0x14);
    OLED_Cmd(0xAF);
    delay_ms(100);
}

static void OLED_Clear(void)
{
    for (uint8_t page = 0; page < 8; page++) {
        OLED_Cmd((uint8_t)(0xB0 + page));
        OLED_Cmd(0x00);
        OLED_Cmd(0x10);
        uint8_t buf[129];
        buf[0] = 0x40;
        for (uint8_t i = 1; i < 129; i++) buf[i] = 0x00;
        I2C1_Write(OLED_ADDR, buf, 129);
    }
}

static const uint8_t font8x8[][8] = {
    /* A */ {0x7C,0x12,0x11,0x12,0x7C,0x00,0x00,0x00},
    /* B */ {0x7F,0x49,0x49,0x49,0x36,0x00,0x00,0x00},
    /* C */ {0x3E,0x41,0x41,0x41,0x22,0x00,0x00,0x00},
    /* D */ {0x7F,0x41,0x41,0x22,0x1C,0x00,0x00,0x00},
    /* E */ {0x7F,0x49,0x49,0x49,0x41,0x00,0x00,0x00},
    /* F */ {0x7F,0x09,0x09,0x09,0x01,0x00,0x00,0x00},
    /* G */ {0x3E,0x41,0x49,0x49,0x7A,0x00,0x00,0x00},
    /* H */ {0x7F,0x08,0x08,0x08,0x7F,0x00,0x00,0x00},
    /* I */ {0x00,0x41,0x7F,0x41,0x00,0x00,0x00,0x00},
    /* J */ {0x20,0x40,0x41,0x3F,0x01,0x00,0x00,0x00},
    /* K */ {0x7F,0x08,0x14,0x22,0x41,0x00,0x00,0x00},
    /* L */ {0x7F,0x40,0x40,0x40,0x40,0x00,0x00,0x00},
    /* M */ {0x7F,0x02,0x0C,0x02,0x7F,0x00,0x00,0x00},
    /* N */ {0x7F,0x04,0x08,0x10,0x7F,0x00,0x00,0x00},
    /* O */ {0x3E,0x41,0x41,0x41,0x3E,0x00,0x00,0x00},
    /* P */ {0x7F,0x09,0x09,0x09,0x06,0x00,0x00,0x00},
    /* Q */ {0x3E,0x41,0x51,0x21,0x5E,0x00,0x00,0x00},
    /* R */ {0x7F,0x09,0x19,0x29,0x46,0x00,0x00,0x00},
    /* S */ {0x46,0x49,0x49,0x49,0x31,0x00,0x00,0x00},
    /* T */ {0x01,0x01,0x7F,0x01,0x01,0x00,0x00,0x00},
    /* U */ {0x3F,0x40,0x40,0x40,0x3F,0x00,0x00,0x00},
    /* V */ {0x1F,0x20,0x40,0x20,0x1F,0x00,0x00,0x00},
    /* W */ {0x3F,0x40,0x38,0x40,0x3F,0x00,0x00,0x00},
   /* X */ {0x63,0x14,0x08,0x14,0x63,0x00,0x00,0x00},
    /* Y */ {0x07,0x08,0x70,0x08,0x07,0x00,0x00,0x00},
    /* Z */ {0x61,0x51,0x49,0x45,0x43,0x00,0x00,0x00},
    /* espacio */ {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
};

static uint8_t CharIndex(char c)
{
    if (c >= 'A' && c <= 'Z') return (uint8_t)(c - 'A');
    if (c >= 'a' && c <= 'z') return (uint8_t)(c - 'a');
    return 26;
}

static void OLED_Print(uint8_t row, uint8_t col, const char *str)
{
    while (*str && col < 128) {
        uint8_t idx = CharIndex(*str++);
        OLED_Cmd((uint8_t)(0xB0 + row));
        OLED_Cmd((uint8_t)(0x00 + (col & 0x0F)));
        OLED_Cmd((uint8_t)(0x10 + (col >> 4)));
        uint8_t buf[9];
        buf[0] = 0x40;
        for (uint8_t i = 0; i < 8; i++) buf[i+1] = font8x8[idx][i];
        I2C1_Write(OLED_ADDR, buf, 9);
        col += 9;
    }
}

typedef enum {
    SCREEN_NONE,
    SCREEN_REPOSO,
    SCREEN_GIRANDO
} screen_state_t;

static screen_state_t screen_current = SCREEN_NONE;

static void OLED_Update(screen_state_t new_state)
{
    if (new_state == screen_current) return;
    screen_current = new_state;
    OLED_Clear();
    switch (new_state) {
        case SCREEN_REPOSO:
            OLED_Print(2, 20, "MOTOR");
            OLED_Print(4, 20, "REPOSO");
            break;
        case SCREEN_GIRANDO:
            OLED_Print(2, 20, "MOTOR");
            OLED_Print(4, 20, "GIRANDO");
            break;
        default:
            break;
    }
}

/* ------------------- MAIN --------------------------------- */
int main(void)
{
    Clock_Init();
    SysTick_Init();
    GPIO_Init_All();
    ADC_Init();
    TIM1_PWM_Init();
    TIM2_Encoder_Init();
    I2C1_Init();

    Motor_Stop();
    Motor_Disable();

    OLED_Init();
    OLED_Clear();
    OLED_Print(3, 10, "INICIANDO");
    delay_ms(1500);

    Motor_Enable();
    OLED_Update(SCREEN_REPOSO);

    uint8_t  flex_debounce = 0;
    uint8_t  motor_running = 0;
    int32_t  encoder_val   = 0;
    (void)encoder_val;

    while (1)
    {
        /* Leer sensor de flexión */
        uint16_t flex_val = ADC_Read();

        /* Debounce */
        if (flex_val < FLEX_THRESHOLD) {
            if (flex_debounce < 5U) flex_debounce++;
        } else {
            if (flex_debounce > 0U) flex_debounce--;
        }

        /* Controlar motor */
        if (flex_debounce >= 4U && !motor_running) {
            motor_running = 1;
            Motor_Set(MOTOR_SPEED, 1U);
            OLED_Update(SCREEN_GIRANDO);
        }
        else if (flex_debounce == 0U && motor_running) {
            motor_running = 0;
            Motor_Stop();
            OLED_Update(SCREEN_REPOSO);
        }

        encoder_val = Encoder_Read_Reset();

        delay_ms(20);
    }
}



