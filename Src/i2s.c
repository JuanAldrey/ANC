#include <stdint.h>
#include "stm32f446xx.h"

#include "i2s.h"

void configureI2S2(void)
{
	// Enable clock for GPIOB
    RCC->AHB1ENR |= (1U << 1);

    // Set GPIOB in alternate function mode
    GPIOB->MODER &= ~((3U << 24) | (3U << 26) | (3U << 30));
    GPIOB->MODER |=  ((2U << 24) | (2U << 26) | (2U << 30));

    // Set AF5 for I2S
    GPIOB->AFR[1] &= ~((15U << 16) | (15U << 20) | (15U << 28));
    GPIOB->AFR[1] |=  ((5U << 16) | (5U << 20) | (5U << 28));

    // Keep both shared I2S clock outputs at low speed.
    GPIOB->OSPEEDR &= ~((3U << 24) | (3U << 26));
    //GPIOB->OSPEEDR |=  (1U << 26);

    // Enable clock for SPI2/I2S
    RCC->APB1ENR |= (1U << 14);

    // Configure I2S
    SPI2->I2SCFGR =
          (1U << 11) |   // I2SMOD = 1  -> I2S mode
          (3U << 8)  |   // I2SCFG = 11 -> Master receive
          (0U << 4)  |   // I2SSTD = 00 -> Philips I2S
          (1U << 1)  |   // DATLEN = 01 -> 24-bit data
          (1U << 0);     // CHLEN = 1    -> 32-bit channel

    // Select PLLI2S_R as APB1 I2S clock source
    RCC->DCKCFGR &= ~(3U << 25);

    // Set I2S CLK to 48kHz
    // First set PLLI2S CLK and then prescale it
    RCC->PLLI2SCFGR &= ~(
          (0x3FU  << 0)  |
          (0x1FFU << 6)  |
          (0x7U   << 28)
    );

    RCC->PLLI2SCFGR |= (
          (8U   << 0)  |
          (192U << 6)  |
          (5U   << 28)
    );

    SPI2->I2SPR &= ~(
          (0xFFU << 0) |
          (1U    << 8) |
          (1U    << 9)
    );

    SPI2->I2SPR |= (
          (12U << 0) |
          (1U  << 8)
    );

    // Turn on PLLI2S and wait for it to stabilize
    RCC->CR |= (1U << 26);
    while (!(RCC->CR & (1U << 27)));
}

void configureI2S3(void)
{
    // Enable clock for GPIOA and GPIOC
    RCC->AHB1ENR |= (1U << 0) | (1U << 2);

    // Set PA4 in alternate function mode
    GPIOA->MODER &= ~(3U << 8);
    GPIOA->MODER |=  (2U << 8);

    // Set PC10 and PC12 in alternate function mode
    GPIOC->MODER &= ~((3U << 20) | (3U << 24));
    GPIOC->MODER |=  ((2U << 20) | (2U << 24));

    // Set AF6 for I2S3
    GPIOA->AFR[0] &= ~(15U << 16);
    GPIOA->AFR[0] |=  (6U << 16);

    GPIOC->AFR[1] &= ~((15U << 8) | (15U << 16));
    GPIOC->AFR[1] |=  ((6U << 8) | (6U << 16));

    // Use very high speed for I2S3 data output
    GPIOC->OSPEEDR &= ~(3U << 24);
    GPIOC->OSPEEDR |=  (3U << 24);

    // Enable clock for SPI3/I2S
    RCC->APB1ENR |= (1U << 15);

    // Configure I2S
    SPI3->I2SCFGR =
          (1U << 11) |   // I2SMOD = 1  -> I2S mode
          (0U << 8)  |   // I2SCFG = 00 -> Slave transmit
          (0U << 4)  |   // I2SSTD = 00 -> Philips I2S
          (1U << 1)  |   // DATLEN = 01 -> 24-bit data
          (1U << 0);     // CHLEN = 1    -> 32-bit channel

}


