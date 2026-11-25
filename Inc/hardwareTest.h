#ifndef HARDWARE_TEST_H
#define HARDWARE_TEST_H

#include <stdint.h>

// Fill the TX buffer with a repeated sine scaled by DAC_TEST_GAIN.
// A zero gain gives digital silence.
void fillDacTestBuffer(void);

// Process complete stereo frames and send the selected mic to both DAC channels.
void copyPassthroughBlock(
    const volatile uint16_t *input,
    volatile uint16_t *output,
    uint32_t wordCount,
    uint32_t micChannel,
    float gain
);

#endif
