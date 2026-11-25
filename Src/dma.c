#include "stm32f446xx.h"
#include "dma.h"

// DMA buffer - 16 bits per requests
// the INMP441 + DMA delivers audio the following way:
// buffer[0] -> 23-8 audio bits L channel					-> Left high
// buffer[1] -> 7-0 audio bits L channel + 8 padding bits	-> Left low
// buffer[2] -> 23-8 audio bits R channel					-> Right high
// buffer[3] -> 7-0 audio bits R channel + 8 padding bits	-> Right low
volatile uint16_t i2sRxBuffer[I2S_RX_BUFFER_LENGTH];
volatile uint16_t i2sTxBuffer[I2S_TX_BUFFER_LENGTH];

volatile uint8_t i2sRxFirstHalfReady = 0;
volatile uint8_t i2sRxSecondHalfReady = 0;

volatile uint8_t i2sTxFirstHalfReady = 0;
volatile uint8_t i2sTxSecondHalfReady = 0;

void configureI2S2DMA(void)
{
	// Enable clock access to DMA1
    RCC->AHB1ENR |= (1U << 21);

    // Disable DMA1 Stream 3 before configuration
    DMA1_Stream3->CR &= ~(1U << 0);

    // Wait until it is actually disabled
    while (DMA1_Stream3->CR & (1U << 0));

    // Configuration
    DMA1_Stream3->CR =
    		(0U << 25) | // CHSEL = 000 -> Channel 0
			(0U << 6)  | // DIR = 00 -> Peripheral-to-memory
			(2U << 16) | // PL = 10 -> Priority high
			(0U << 9)  | // PINC = 0 -> Peripheral address pointer fixed
			(1U << 10) | // MINC = 1 -> Memory address incremented after each data transfer
			(1U << 11) | // PSIZE = 01 -> Half-word (16-bit)
			(1U << 13) | // MSIZE = 01 -> Half-word (16-bit)
			(1U << 8)  | // CIRC = 1 -> Circular mode ON
			(1U << 3)  | // HTIE = 1 -> Enable half-transfer interrupt
			(1U << 4);   // TCIE = 1 -> Enable transfer complete interrupt

	// Set NDTR of stream 3 to buffer length
	DMA1_Stream3->NDTR = I2S_RX_BUFFER_LENGTH;

	// Set peripheral address register
	DMA1_Stream3->PAR   = (uint32_t)&SPI2->DR;

	// Set memory address register
	DMA1_Stream3->M0AR  = (uint32_t)i2sRxBuffer;

	// Set direct mode
	DMA1_Stream3->FCR &= ~(1U << 2);   // DMDIS = 0 -> Direct mode

	// Clear flags
	DMA1->LIFCR =
	      (1U << 22) |
	      (1U << 24) |
	      (1U << 25) |
	      (1U << 26) |
	      (1U << 27);

	// Enable interrupts on NVIC for DMA1 Stream 3
	NVIC->ISER[0] |= (1U << 14);

	// Enable DMA1 Stream 3
	DMA1_Stream3->CR |= (1U << 0);
}

void configureI2S3DMA(void)
{
	// Enable clock access to DMA1
	RCC->AHB1ENR |= (1U << 21);

	// Disable DMA1 Stream 5 before configuration
	DMA1_Stream5->CR &= ~(1U << 0);

	// Wait until it is actually disabled
	while (DMA1_Stream5->CR & (1U << 0));

    // Configuration
    DMA1_Stream5->CR =
    		(0U << 25) | // CHSEL = 000 -> Channel 0
			(1U << 6)  | // DIR = 01 -> Memory-to-peripheral
			(2U << 16) | // PL = 10 -> Priority high
			(0U << 9)  | // PINC = 0 -> Peripheral address pointer fixed
			(1U << 10) | // MINC = 1 -> Memory address incremented after each data transfer
			(1U << 11) | // PSIZE = 01 -> Half-word (16-bit)
			(1U << 13) | // MSIZE = 01 -> Half-word (16-bit)
			(1U << 8)  | // CIRC = 1 -> Circular mode ON
			(1U << 3)  | // HTIE = 1 -> Enable half-transfer interrupt
			(1U << 4);   // TCIE = 1 -> Enable transfer complete interrupt

    // Set NDTR of stream 5 to buffer length
	DMA1_Stream5->NDTR = I2S_TX_BUFFER_LENGTH;

	// Set peripheral address register
	DMA1_Stream5->PAR   = (uint32_t)&SPI3->DR;

	// Set memory address register
	DMA1_Stream5->M0AR  = (uint32_t)i2sTxBuffer;

	// Set direct mode
	DMA1_Stream5->FCR &= ~(1U << 2);   // DMDIS = 0 -> Direct mode

	// Clear flags
	DMA1->HIFCR =
		  (1U << 6) |
		  (1U << 8) |
		  (1U << 9) |
		  (1U << 10) |
		  (1U << 11);

	// Enable interrupts on NVIC for DMA1 Stream 5
	NVIC->ISER[0] |= (1U << 16);

	// Enable DMA1 Stream 5
	DMA1_Stream5->CR |= (1U << 0);

}

void DMA1_Stream3_IRQHandler(void)
{
    if (DMA1->LISR & (1U << 26))
    {
        DMA1->LIFCR = (1U << 26);
        i2sRxFirstHalfReady = 1;
    }

    if (DMA1->LISR & (1U << 27))
    {
        DMA1->LIFCR = (1U << 27);
        i2sRxSecondHalfReady = 1;
    }
}

void DMA1_Stream5_IRQHandler(void)
{
    if (DMA1->HISR & (1U << 10))
    {
        DMA1->HIFCR = (1U << 10);
        i2sTxFirstHalfReady = 1;
    }

    if (DMA1->HISR & (1U << 11))
    {
        DMA1->HIFCR = (1U << 11);
        i2sTxSecondHalfReady = 1;
    }
}

