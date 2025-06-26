#ifndef PQ_COM_FORMAT_TYPES_H
#define PQ_COM_FORMAT_TYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(PQ_COM_FORMAT_DLL) && defined(_WIN32)
#ifdef PQ_COM_FORMAT_BUILDING_DLL
#define PQ_COM_FORMAT_API __declspec(dllexport)
#else
#define PQ_COM_FORMAT_API __declspec(dllimport)
#endif
#else
#define PQ_COM_FORMAT_API
#endif

typedef enum {
  PQ_COM_FORMAT_SUCCESS = 0,
  PQ_COM_FORMAT_ERROR_INVALID_PARAM = -1,
  PQ_COM_FORMAT_ERROR_BUFFER_TOO_SMALL = -2,
  PQ_COM_FORMAT_ERROR_INVALID_FORMAT = -3,
  PQ_COM_FORMAT_ERROR_MEMORY = -4,
  PQ_COM_FORMAT_ERROR_CHECKSUM = -5
} pq_com_format_result_t;

typedef struct {
  uint8_t data[PQ_COM_FORMAT_MAX_PACKET_SIZE];
  uint16_t size;
  uint16_t capacity;
} pq_com_format_buffer_t;

typedef struct {
  uint16_t version;
  uint16_t length;
  pq_com_format_buffer_t payload;
  uint32_t checksum;
} pq_com_format_t;

#ifdef __cplusplus
}
#endif

#endif
