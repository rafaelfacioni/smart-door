/*

  _    _ _______   __  __ _____ _____ _____   ____  _   _
 | |  | |__   __| |  \/  |_   _/ ____|  __ \ / __ \| \ | |
 | |__| |  | |    | \  / | | || |    | |__) | |  | |  \| |
 |  __  |  | |    | |\/| | | || |    |  _  /| |  | | . ` |
 | |  | |  | |    | |  | |_| || |____| | \ \| |__| | |\  |
 |_|  |_|  |_|    |_|  |_|_____\_____|_|  \_\\____/|_| \_|
 =================== Advanced R&D ========================

 Copyright (c) 2023 HT Micron Semicondutores S.A.
 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at
 http://www.apache.org/licenses/LICENSE-2.0
 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.

*/

/*!
 * \file HT_Peripheral_Config.h
 * \brief Peripheral configuration for MQTT Example.
  * \author HT Micron Advanced R&D,
 *         Hêndrick Bataglin Gonçalves, Christian Roberto Lehmen,  Matheus da Silva Zorzeto, Felipe Kalinski Ferreira,
 *         Leandro Borges, Mauricio Carlotto Ribeiro, Henrique Kuhn, Cleber Haack, Eduardo Mendel
 *         Gleiser Alvarez Arrojo
 * 
 * \link https://github.com/htmicron
 * \version 0.1
 * \date September 15, 2023
 */

#ifndef __HT_PERIPHERAL_CONFIG_H__
#define __HT_PERIPHERAL_CONFIG_H__

#include "qcx212.h"

/*
----------------------------------------------------  Table 1. GPIO Table. -------------------------------------------------------------
___________________________________________________________________________________________________________________________________________________________________________________________
| Pad ID |       GPIO Number    |  Pin Number  | Instance | Pull (default:options) |       AF0      |      AF1      |    AF2      |      AF3      |  AF4 |   AF5  |   AF6  |      AF7      |
|--------|----------------------|--------------|----------|------------------------|----------------|---------------|-------------|---------------|------|--------|--------|---------------|
|   12   |        GPIO1         |      1       |    0     |      B-PU:nppd         |  GPIO1         |      –	      |  UART2_TXD  |      –        |   –  |    –   |    –   |      –        | 
|   13   |        GPIO2         |      2       |    0     |      B-PU:nppd         |  GPIO2/Timer0  |  UART0_RTSn   |  UART2_RXD  |    SPI1_SSn0  |   –  |  PWM0  |    –   |      –        |
|   14   |        GPIO3         |      3       |    0     |      B-PU:nppd         |  GPIO3         |  UART0_CTSn   |  UART2_TXD  |    SPI1_MOSI  |   –  |  PWM1  |    –   |      –        |
|   15   |        GPIO4         |      4       |    0     |      B-PU:nppd         |  GPIO4         |  UART0_RXD    |  I2C1_SDA   |    SPI1_MISO  |   –  |  PWM2  |    –   |      –        |
|   16   |        GPIO5         |      5       |    0     |      B-PU:nppd         |  GPIO5         |  UART0_TXD    |  I2C1_SCL   |    SPI1_SCLK  |   –  |  PWM3  |    –   |      –        |
|   17   |        GPIO6         |      6       |    0     |      B-PU:nppd         |  GPIO6/Timer1  |  SPI0_SSn0    |  I2C0_SDA   |    UART1_RTSn |   –  |  PWM4  |    –   |      –        |
|   18   |        GPIO7         |      7       |    0     |      B-PU:nppd         |  GPIO7         |  SPI0_MOSI    |  I2C0_SCL   |    UART1_CTSn |   –  |  PWM5  |    –   |      –        |
|   19   |        GPIO13        |      13      |    0     |      B-PU:nppd         |  GPIO13        |  SPI0_MISO    |  I2C1_SDA   |    UART1_RXD  |   –  |  PWM0  |    –   |      –        |
|   20   |        GPIO12        |      12      |    0     |      B-PU:nppd         |  GPIO12        |  SPI0_SCLK    |  I2C1_SCL   |    UART1_TXD  |   –  |  PWM1  |    –   |      –        |
|   25   |        GPIO10        |      10      |    0     |      B-PU:nppd         |  GPIO10/Timer3 |  I2C0_SCL     |      –      |    SPI1_SSn1  |   –  |  PWM0  |    –   |      –        |
|   9    |        SWCLK         |      2       |    1     |      B-PU:nppd         |  SWCLK         |      –        |  UART2_RXD  |    UART1_RTSn |   –  |  PWM4  |    –   |    GPIO18     |
|   10   |        SWDIO         |      3       |    1     |      B-PU:nppd         |  SWDIO         |      –        |  UART2_TXD  |    UART1_CTSn |   –  |  PWM5  |    –   |    GPIO19     |
|   31   |   AON_GPIO0 (GPIO20)	|      4       |    1     |      DIO-PD:nppu       |  GPIO20/Timer5 |      –        |      –      |      –        |   –  |    –   |    –   |      –        |
____________________________________________________________________________________________________________________________________________________________________________________________

-> B  : Bidirectional digital with CMOS input		
-> DIO: Digital input output		
-> NP : pdpu = default no-pull with programmable options following the colon (:)		
-> PD : nppu = default pull-down with programmable options following the colon (:)		
-> PU : nppd = default pull-up with programmable options following the colon (:)		
*/

/* Defines  ------------------------------------------------------------------*/

/*  Peripheral IO Mode Select, Must Configure First !!!
    Note, when receiver works in DMA_MODE, interrupt is also enabled to transfer tailing bytes.
*/

#define POLLING_MODE            0x1
#define DMA_MODE                0x2
#define IRQ_MODE                0x3
#define UNILOG_MODE             0x4

#define RTE_UART0_TX_IO_MODE    UNILOG_MODE
#define RTE_UART0_RX_IO_MODE    POLLING_MODE
#define USART0_RX_TRIG_LVL      (30)

#define RTE_UART1_TX_IO_MODE    POLLING_MODE
#define RTE_UART1_RX_IO_MODE    POLLING_MODE

#define RTE_UART2_TX_IO_MODE    POLLING_MODE
#define RTE_UART2_RX_IO_MODE    POLLING_MODE

#define RTE_SPI0_IO_MODE        POLLING_MODE
#define RTE_SPI1_IO_MODE        POLLING_MODE

#define I2C0_INIT_MODE          POLLING_MODE
#define I2C1_INIT_MODE          POLLING_MODE


// I2C0 (Inter-integrated Circuit Interface) [Driver_I2C0]
// Configuration settings for Driver_I2C0 in component ::Drivers:I2C
#define RTE_I2C0                        1

// { PAD_PIN18},  // 0 : gpio7  / 2 : I2C0 SCL
// { PAD_PIN26},  // 0 : gpio6  / 2 : I2C0 SDA
#define RTE_I2C0_SCL_PAD_ID                18
#define RTE_I2C0_SCL_FUNC               PAD_MuxAlt2

#define RTE_I2C0_SDA_PAD_ID                17
#define RTE_I2C0_SDA_FUNC               PAD_MuxAlt2

// DMA
//   Tx
//     Channel     <0=>0 <1=>1 <2=>2 <3=>3 <4=>4 <5=>5 <6=>6 <7=>7
#define RTE_I2C0_DMA_TX_EN              0
#define RTE_I2C0_DMA_TX_REQID           DMA_RequestI2C0TX
//   Rx
//     Channel     <0=>0 <1=>1 <2=>2 <3=>3 <4=>4 <5=>5 <6=>6 <7=>7
#define RTE_I2C0_DMA_RX_EN              0
#define RTE_I2C0_DMA_RX_REQID           DMA_RequestI2C0RX

// I2C1 (Inter-integrated Circuit Interface) [Driver_I2C1]
// Configuration settings for Driver_I2C1 in component ::Drivers:I2C
#define RTE_I2C1                        0

// { PAD_PIN16},  // 0 : gpio5 / 1 : I2C1 SCL
// { PAD_PIN15},  // 0 : gpio4  / 1 : I2C1 SDA
#define RTE_I2C1_SCL_PAD_ID                16
#define RTE_I2C1_SCL_FUNC               PAD_MuxAlt2

#define RTE_I2C1_SDA_PAD_ID                15
#define RTE_I2C1_SDA_FUNC               PAD_MuxAlt2

// DMA
//   Tx
//     Channel     <0=>0 <1=>1 <2=>2 <3=>3 <4=>4 <5=>5 <6=>6 <7=>7
#define RTE_I2C1_DMA_TX_EN              0
#define RTE_I2C1_DMA_TX_REQID           DMA_RequestI2C1TX
//   Rx
//     Channel     <0=>0 <1=>1 <2=>2 <3=>3 <4=>4 <5=>5 <6=>6 <7=>7
#define RTE_I2C1_DMA_RX_EN              0
#define RTE_I2C1_DMA_RX_REQID           DMA_RequestI2C1RX


// UART0 (Universal asynchronous receiver transmitter) [Driver_USART0]
// Configuration settings for Driver_USART0 in component ::Drivers:USART
#define RTE_UART0                       0
#define RTE_UART0_CTS_PIN_EN            0
#define RTE_UART0_RTS_PIN_EN            0

// { PAD_PIN13},  // 0 : gpio2 / 1 : UART0 RTSn / 3 : SPI1 SSn
// { PAD_PIN14},  // 0 : gpio3 / 1 : UART0 CTSn / 3 : SPI1 MOSI
// { PAD_PIN15},  // 0 : gpio4 / 1 : UART0 RXD  / 3 : SPI1 MISO
// { PAD_PIN16},  // 0 : gpio5 / 1 : UART0 TXD  / 3 : SPI1 SCLK
#define RTE_UART0_RTS_PAD_ID               13
#define RTE_UART0_RTS_FUNC              PAD_MuxAlt1

#define RTE_UART0_CTS_PAD_ID               14
#define RTE_UART0_CTS_FUNC              PAD_MuxAlt1

#define RTE_UART0_RX_PAD_ID                15
#define RTE_UART0_RX_FUNC               PAD_MuxAlt1

#define RTE_UART0_TX_PAD_ID                16
#define RTE_UART0_TX_FUNC               PAD_MuxAlt1

// DMA
//  Tx
//    Channel     <0=>0 <1=>1 <2=>2 <3=>3 <4=>4 <5=>5 <6=>6 <7=>7
#define RTE_UART0_DMA_TX_REQID          DMA_RequestUSART0TX
//  Rx
//    Channel    <0=>0 <1=>1 <2=>2 <3=>3 <4=>4 <5=>5 <6=>6 <7=>7
#define RTE_UART0_DMA_RX_REQID          DMA_RequestUSART0RX

// UART1 (Universal asynchronous receiver transmitter) [Driver_USART1]
// Configuration settings for Driver_USART1 in component ::Drivers:USART
#define RTE_UART1                       1
#define RTE_UART1_CTS_PIN_EN            0
#define RTE_UART1_RTS_PIN_EN            0
// { PAD_PIN17},  // 0 : gpio6   / 3 : UART1 RTS
// { PAD_PIN18},  // 0 : gpio7   / 3 : UART1 CTS
// { PAD_PIN19},  // 0 : gpio13  / 3 : UART1 RXD
// { PAD_PIN20},  // 0 : gpio12  / 3 : UART1 TXD
#define RTE_UART1_RTS_PAD_ID               17
#define RTE_UART1_RTS_FUNC              PAD_MuxAlt3

#define RTE_UART1_CTS_PAD_ID               18
#define RTE_UART1_CTS_FUNC              PAD_MuxAlt3

#define RTE_UART1_RX_PAD_ID                19
#define RTE_UART1_RX_FUNC               PAD_MuxAlt3

#define RTE_UART1_TX_PAD_ID                20
#define RTE_UART1_TX_FUNC               PAD_MuxAlt3

// DMA
//   Tx
//     Channel     <0=>0 <1=>1 <2=>2 <3=>3 <4=>4 <5=>5 <6=>6 <7=>7
#define RTE_UART1_DMA_TX_REQID          DMA_RequestUSART1TX
//   Rx
//     Channel    <0=>0 <1=>1 <2=>2 <3=>3 <4=>4 <5=>5 <6=>6 <7=>7
#define RTE_UART1_DMA_RX_REQID          DMA_RequestUSART1RX

// UART2 (Universal asynchronous receiver transmitter) [Driver_USART2]
// Configuration settings for Driver_USART2 in component ::Drivers:USART
#define RTE_UART2                       0
#define RTE_UART2_CTS_PIN_EN            0
#define RTE_UART2_RTS_PIN_EN            0

// { PAD_PIN13},  // 0 : gpio2 / 2 : UART2 RXD
// { PAD_PIN14},  // 0 : gpio3 / 2 : UART2 TXD
#define RTE_UART2_RX_PAD_ID                13   
#define RTE_UART2_RX_FUNC               PAD_MuxAlt2

#define RTE_UART2_TX_PAD_ID                14  
#define RTE_UART2_TX_FUNC               PAD_MuxAlt2


// DMA
//   Tx
//     Channel     <0=>0 <1=>1 <2=>2 <3=>3 <4=>4 <5=>5 <6=>6 <7=>7
#define RTE_UART2_DMA_TX_REQID          DMA_RequestUSART2TX
//   Rx
//     Channel    <0=>0 <1=>1 <2=>2 <3=>3 <4=>4 <5=>5 <6=>6 <7=>7
#define RTE_UART2_DMA_RX_REQID          DMA_RequestUSART2RX

#define HAL_USART0_SELECT 0
#define HAL_USART1_SELECT 1
#define HAL_USART2_SELECT 2

#define USART_PRINT_SELECT  HAL_USART1_SELECT
#define USART_UNILOG_SELECT HAL_USART0_SELECT

// SPI0 (Serial Peripheral Interface) [Driver_SPI0]
// Configuration settings for Driver_SPI0 in component ::Drivers:SPI
#define RTE_SPI0                        0

// { PAD_PIN17},  // 0 : gpio6  / 1 : SPI0 SSn
// { PAD_PIN18},  // 0 : gpio7  / 1 : SPI0 MOSI
// { PAD_PIN19},  // 0 : gpio13 / 1 : SPI0 MISO
// { PAD_PIN20},  // 0 : gpio12 / 1 : SPI0 SCLK
#define RTE_SPI0_SSN_BIT                   17
#define RTE_SPI0_SSN_FUNC               PAD_MuxAlt1

#define RTE_SPI0_MOSI_PAD_ID               18
#define RTE_SPI0_MOSI_FUNC              PAD_MuxAlt1

#define RTE_SPI0_MISO_PAD_ID               19
#define RTE_SPI0_MISO_FUNC              PAD_MuxAlt1

#define RTE_SPI0_SCLK_PAD_ID               20
#define RTE_SPI0_SCLK_FUNC              PAD_MuxAlt1

#define RTE_SPI0_SSN_GPIO_INSTANCE      1
#define RTE_SPI0_SSN_GPIO_INDEX         0

// DMA
//   Tx
//     Channel     <0=>0 <1=>1 <2=>2 <3=>3 <4=>4 <5=>5 <6=>6 <7=>7
#define RTE_SPI0_DMA_TX_REQID           DMA_RequestSPI0TX

//   Rx
//     Channel     <0=>0 <1=>1 <2=>2 <3=>3 <4=>4 <5=>5 <6=>6 <7=>7
#define RTE_SPI0_DMA_RX_REQID           DMA_RequestSPI0RX

// SPI1 (Serial Peripheral Interface) [Driver_SPI1]
// Configuration settings for Driver_SPI1 in component ::Drivers:SPI
#define RTE_SPI1                        1

// { PAD_PIN13},  // 0 : gpio2 / 1 : UART0 RTSn / 3 : SPI1 SSn
// { PAD_PIN14},  // 0 : gpio3 / 1 : UART0 CTSn / 3 : SPI1 MOSI
// { PAD_PIN15},  // 0 : gpio4 / 1 : UART0 RXD  / 3 : SPI1 MISO
// { PAD_PIN16},  // 0 : gpio5 / 1 : UART0 TXD  / 3 : SPI1 SCLK
#define RTE_SPI1_SSN_PAD_ID                13
#define RTE_SPI1_SSN_FUNC               PAD_MuxAlt3

#define RTE_SPI1_MOSI_PAD_ID               14
#define RTE_SPI1_MOSI_FUNC              PAD_MuxAlt3

#define RTE_SPI1_MISO_PAD_ID               15
#define RTE_SPI1_MISO_FUNC              PAD_MuxAlt3

#define RTE_SPI1_SCLK_PAD_ID               16
#define RTE_SPI1_SCLK_FUNC              PAD_MuxAlt3

#define RTE_SPI1_SSN_GPIO_INSTANCE      0
#define RTE_SPI1_SSN_GPIO_INDEX         2

// DMA
//   Tx
//     Channel     <0=>0 <1=>1 <2=>2 <3=>3 <4=>4 <5=>5 <6=>6 <7=>7
#define RTE_SPI1_DMA_TX_REQID           DMA_RequestSPI1TX

//   Rx
//     Channel     <0=>0 <1=>1 <2=>2 <3=>3 <4=>4 <5=>5 <6=>6 <7=>7
#define RTE_SPI1_DMA_RX_REQID           DMA_RequestSPI1RX


// PWM0 Controller [Driver_PWM0]
// Configuration settings for Driver_PWM0 in component ::Drivers:PWM
#define RTE_PWM                         1

#define EFUSE_INIT_MODE POLLING_MODE
#define L2CTLS_INIT_MODE POLLING_MODE

#define FLASH_BARE_RW_MODE 1

#endif  /* __HT_PERIPHERAL_CONFIG_H__ */

/************************ HT Micron Semicondutores S.A *****END OF FILE****/
