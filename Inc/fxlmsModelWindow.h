#ifndef FXLMS_MODEL_WINDOW_H
#define FXLMS_MODEL_WINDOW_H

// EDITAR AQUI EN CUBEIDE. Guardar, recompilar y reiniciar el MCU.
// Rangos [FIRST, END), en muestras a 8 kHz. END es exclusivo.
// G: camino a ERR. F: realimentacion al microfono REF.
#define GHAT_FIRST_TAP 0U
#define GHAT_END_TAP   64U
#define FHAT_FIRST_TAP 0U
#define FHAT_END_TAP   512U

// END incluye el retardo desde t=0, no solo la cola posterior al pico.
// [0,FIRST) queda en cero. Las llegadas no se desplazan al recortar.
// Corte directo de la base exportada, sin fade adicional.
// Con estas ventanas, W_LENGTH en fxlms.h es 576 (=64+512).

#endif
