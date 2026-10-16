# Arquitectura_Brazo

Firmware de prueba para el sistema electromecánico de asistencia de agarre (3 motores con encoder, sensores y OLED).

## Hardware
- Black Pill STM32F401 (WeAct), programada por ST-Link V2 (SWD)
- 2 drivers TB6612FNG (3 motores N20 con encoder)
- Sensor de corriente INA3221 (I2C), OLED SSD1315 128x64 (I2C)
- Sensor de flexión, módulo EMG analógico (TL084), LED externo
- Esquemático: `docs/esquematico.pdf`. Mapa de pines: `hardware/pinout.md`

## Herramientas
STM32CubeIDE, escribiendo registros directamente (sin HAL).

## Cómo usar cada prueba
Cada carpeta de `tests/` contiene un `main.c`. Crea (o reutiliza) un proyecto de CubeIDE para la STM32F401 y reemplaza su `main.c` con el de la prueba. Los archivos de proyecto de CubeIDE no se incluyen porque no se pudieron generar y verificar aquí.

## Estado de las pruebas
| Prueba | Estado |
|---|---|
| 01_led | Código escrito, **sin compilar ni probar** |
| 02 a 07 | Pendientes |

Actualiza esta tabla solo cuando una prueba haya sido verificada físicamente.

## Integración
`integration/etapa_XX/` guarda cada etapa completa, partiendo siempre de una versión ya probada.
