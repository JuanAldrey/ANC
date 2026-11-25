#ifndef SAMPLE_RATE_H
#define SAMPLE_RATE_H

#include <stdint.h>
#include "dma.h"

// These filters are designed specifically for 48 kHz I/O and 8 kHz processing.
#define SAMPLE_RATE_IO_HZ 48000U
#define SAMPLE_RATE_FACTOR 6U
#define SAMPLE_RATE_CONTROL_HZ (SAMPLE_RATE_IO_HZ / SAMPLE_RATE_FACTOR)
#define SAMPLE_RATE_IO_FRAMES (I2S_RX_BUFFER_LENGTH / 8U)
#define SAMPLE_RATE_CONTROL_FRAMES (SAMPLE_RATE_IO_FRAMES / SAMPLE_RATE_FACTOR)

#if I2S_RX_BUFFER_LENGTH != I2S_TX_BUFFER_LENGTH || (I2S_RX_BUFFER_LENGTH % 8U) != 0U
#error "Rate conversion needs equal DMA buffers with complete stereo half-buffers"
#endif

#if (SAMPLE_RATE_IO_FRAMES % SAMPLE_RATE_FACTOR) != 0U
#error "Each DMA half must contain a multiple of six frames"
#endif

// Reset once before streaming. Filter histories persist between DMA halves.
void initSampleRateConversion(void);

// Decode and filter both PCM24 channels, then keep one sample in six.
void decimateMicrophones(
    const volatile uint16_t *input,
    float *reference,
    float *error
);

// Insert zeros, reconstruct at 48 kHz and pack PCM24 to both DAC channels.
void interpolateOutput(const float *control, volatile uint16_t *output, float gain);

#endif
