# Mapa de pines (Black Pill STM32F401, esquemático Brazito_team V1.0, 2026-10-12)

| Función | Pin | Periférico | Estado |
|---|---|---|---|
| Driver1 PWMA / AIN1 / AIN2 (Motor1) | PA8 / PB12 / PB13 | TIM1_CH1 / GPIO / GPIO | Visto en esquemático |
| Driver1 PWMB / BIN1 / BIN2 (Motor2) | PA9 / PB14 / PB15 | TIM1_CH2 / GPIO / GPIO | Visto en esquemático |
| Driver2 PWMA / AIN1 / AIN2 (Motor3) | PA10 / PB0 / PB1 | TIM1_CH3 / GPIO / GPIO | Visto en esquemático |
| nSTBY (ambos drivers) | PB10 | GPIO | Visto en esquemático |
| Encoder Motor1 (A, B) | PA6, PA7 | TIM3 CH1/CH2 (AF2) | Visto en esquemático |
| Encoder Motor2 (A, B) | PB6, PB7 | TIM4 CH1/CH2 (AF2) | Visto en esquemático |
| Encoder Motor3 (A, B) | PA15, PB3 | TIM2 CH1/CH2 (AF1) | Visto en esquemático |
| I2C SCL / SDA | PB8 / PB9 | I2C1 (AF4) | Pines vistos; asignación SCL/SDA por orden del módulo, por confirmar |
| Flexor | PA4 | ADC1_IN4 | Visto en esquemático |
| EMG | PA2 | ADC1_IN2 | Visto; falta verificar voltaje de salida (<= 3.3 V) |
| LED externo (R1 330 ohm) | PB5 | GPIO | Visto en esquemático |

## Pendientes
- El esquemático dibuja un INA219; el sensor real será un INA3221 (falta modelo de módulo, shunt y dirección I2C).
- OLED SSD1315 128x64 por I2C: la bornera "Pantalla" es de 3 pines y no está conectada.
- CN1, P1, U3 y conectores de motor/encoder sin definir.
- VMot = step-down a 6 V.
