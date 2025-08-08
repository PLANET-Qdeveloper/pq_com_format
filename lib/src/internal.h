#ifndef PQ_COM_FORMAT_INTERNAL_H
#define PQ_COM_FORMAT_INTERNAL_H

#include "pq_com_format/pq_com_format.h"

#ifdef __cplusplus
extern "C" {
#endif

// Internal utility functions and definitions

#define PQ_COM_FORMAT_CRC_POLYNOMIAL 0x1021U
#define PQ_COM_FORMAT_CRC_INITIAL    0xFFFFU

// Helper macros for embedded optimization
#define PQ_COM_FORMAT_LIKELY(x)   (x)
#define PQ_COM_FORMAT_UNLIKELY(x) (x)

// Reset function for state machines (thread-safe alternative)
typedef struct {
    uint8_t decode_state;
    uint8_t is_stuffed;
} pq_com_decode_context_t;

typedef struct {
    uint8_t stuffed_byte;
    uint8_t stuffing_active;
} pq_com_encode_context_t;

#ifdef __cplusplus
}
#endif

#endif
