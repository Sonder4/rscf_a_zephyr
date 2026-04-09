# RSCF A Hardware Map Reference

This reference captures the hardware facts extracted from the legacy
`RSCF_A.ioc` and `Docs/ioc_json/*.json` files.

## Clock

- HSE: `12 MHz`
- SYSCLK: `168 MHz`
- AHB: `168 MHz`
- APB1: `42 MHz`
- APB2: `84 MHz`
- USB clock: `48 MHz`
- PLL: `M=6`, `N=168`, `Q=7`

## UART and USART

- `USART1`: `PB6/PB7`
- `USART2`: `PD5/PD6`
- `USART3`: `PD8/PD9`
- `USART6`: `PG14/PG9`
- `UART7`: `PE7/PE8`
- `UART8`: `PE0/PE1`

## CAN

- `CAN1`: `PD0/PD1`, `1 Mbps`
- `CAN2`: `PB12/PB13`, `1 Mbps`

## SPI

- `SPI4`: `PE4/PE5/PE6/PE12`
- `SPI5`: `PF6/PF7/PF8/PF9`

## I2C

- `I2C2`: `PF0/PF1`

## USB

- `USB OTG FS`: `PA11/PA12`
- device class: `CDC`

## GPIO

- `PF14`: `LED_G`
- `PB2`: user key / external interrupt
