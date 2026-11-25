#include "stm32f446xx.h"
#include "clock.h"

void configureSystemClock(void)
{
	// Enable power controller clock
	RCC->APB1ENR |= (1U << 28);

	// Force completion of the clock-enable write
	(void)RCC->APB1ENR;

	// Voltage scale 1
	PWR->CR &= ~(3U << 14);
	PWR->CR |=  (3U << 14);

    // Flash: 5 wait states, instruction cache,
    // data cache and prefetch enabled
    FLASH->ACR =
          (5U << 0)
        | (1U << 8)
        | (1U << 9)
        | (1U << 10);

    // Disable main PLL before configuration
    RCC->CR &= ~(1U << 24);

    while (RCC->CR & (1U << 25))
    {
    }

    // Main PLL:
    // HSI = 16 MHz
    // PLL input = 16 MHz / 8 = 2 MHz
    // PLL VCO = 2 MHz * 168 = 336 MHz
    // SYSCLK = 336 MHz / 2 = 168 MHz
    RCC->PLLCFGR =
          (8U   << 0)    // PLLM = 8
        | (168U << 6)    // PLLN = 168
        | (0U   << 16)   // PLLP = 00 -> divide by 2
        | (0U   << 22)   // PLLSRC = HSI
        | (7U   << 24);  // PLLQ = 7

    // AHB = 168 MHz
    // APB1 = 42 MHz
    // APB2 = 84 MHz
    RCC->CFGR &= ~(
          (0xFU << 4)
        | (0x7U << 10)
        | (0x7U << 13)
    );

    RCC->CFGR |=
          (5U << 10)     // APB1 divide by 4
        | (4U << 13);    // APB2 divide by 2

    // Enable PLL
    RCC->CR |= (1U << 24);

    while (!(RCC->CR & (1U << 25)))
    {
    }

    // Now voltage scale 1 becomes active
    while (!(PWR->CSR & (1U << 14)))
    {
    }

    // Select PLL as system clock
    RCC->CFGR &= ~(3U << 0);
    RCC->CFGR |=  (2U << 0);

    // Wait until PLL is used as system clock
    while (((RCC->CFGR >> 2) & 3U) != 2U)
    {
    }

    // Update software clock value
    SystemCoreClock = 168000000U;

}
