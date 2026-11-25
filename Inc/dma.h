#ifndef DMA_H
#define DMA_H

#include <stdint.h>

// Stereo PCM24 uses four halfwords per frame: 48 frames per DMA half.
#define I2S_RX_BUFFER_LENGTH 384
#define I2S_TX_BUFFER_LENGTH 384

extern volatile uint16_t i2sRxBuffer[I2S_RX_BUFFER_LENGTH];
extern volatile uint16_t i2sTxBuffer[I2S_TX_BUFFER_LENGTH];

extern volatile uint8_t i2sRxFirstHalfReady;
extern volatile uint8_t i2sRxSecondHalfReady;

extern volatile uint8_t i2sTxFirstHalfReady;
extern volatile uint8_t i2sTxSecondHalfReady;

void configureI2S2DMA(void);
void configureI2S3DMA(void);

#endif
