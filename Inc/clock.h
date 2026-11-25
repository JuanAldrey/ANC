#ifndef CLOCK_H
#define CLOCK_H

// Configura la CPU a 168 MHz y actualiza SystemCoreClock.
// El procesamiento de audio se agenda mediante DMA, sin SysTick.
void configureSystemClock(void);

#endif
