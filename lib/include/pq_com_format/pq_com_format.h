#ifndef PQ_COM_FORMAT_H
#define PQ_COM_FORMAT_H

#include "config.h"
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

PQ_COM_FORMAT_API const char *pq_com_format_version_string(void);
PQ_COM_FORMAT_API uint32_t pq_com_format_version_number(void);
PQ_COM_FORMAT_API const char *
pq_com_format_result_string(pq_com_format_result_t result);

PQ_COM_FORMAT_API void
pq_com_format_buffer_init(pq_com_format_buffer_t *buffer);

PQ_COM_FORMAT_API void
pq_com_format_buffer_clear(pq_com_format_buffer_t *buffer);

PQ_COM_FORMAT_API pq_com_format_result_t pq_com_format_buffer_ensure_capacity(
    pq_com_format_buffer_t *buffer, size_t required_size);

PQ_COM_FORMAT_API uint32_t pq_com_format_calculate_checksum(const uint8_t *data,
                                                            size_t size);

PQ_COM_FORMAT_API pq_com_format_result_t pq_com_format_init(
    pq_com_format_t *packet, uint8_t *payload, uint16_t payload_size);

PQ_COM_FORMAT_API pq_com_format_result_t
pq_com_format_clear(pq_com_format_t *packet);

PQ_COM_FORMAT_API pq_com_format_result_t pq_com_format_set(
    pq_com_format_t *packet, const uint8_t *payload, uint16_t payload_size);

PQ_COM_FORMAT_API pq_com_format_result_t pq_com_format_encode(
    pq_com_format_t *packet, uint8_t *output, uint16_t output_size);

PQ_COM_FORMAT_API pq_com_format_result_t pq_com_format_decode(
    pq_com_format_t *packet, const uint8_t *input, uint16_t input_size);

#ifdef __cplusplus
}
#endif

#endif
