#ifndef PQ_COM_FORMAT_CONFIG_H
#define PQ_COM_FORMAT_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef PQ_COM_FORMAT_MAX_PACKET_SIZE
#define PQ_COM_FORMAT_MAX_PACKET_SIZE 1024
#endif
#ifndef PQ_COM_FORMAT_MAGIC
#define PQ_COM_FORMAT_MAGIC 0x50514346
#endif
#ifndef PQ_COM_FORMAT_VERSION
#define PQ_COM_FORMAT_VERSION 1
#endif
#ifndef PQ_COM_FORMAT_HEADER_SIZE
#define PQ_COM_FORMAT_HEADER_SIZE                                              \
  (sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint16_t))
#endif
#ifndef PQ_COM_FORMAT_FOOTER_SIZE
#define PQ_COM_FORMAT_FOOTER_SIZE (sizeof(uint32_t))
#endif
#ifndef PQ_COM_FORMAT_CRC32_POLY
#define PQ_COM_FORMAT_CRC32_POLY 0xEDB88320
#endif
#ifndef PQ_COM_FORMAT_CRC32_INIT
#define PQ_COM_FORMAT_CRC32_INIT 0xFFFFFFFF
#endif
#ifndef PQ_COM_FORMAT_CRC32_XOROUT
#define PQ_COM_FORMAT_CRC32_XOROUT 0xFFFFFFFF
#endif

#ifdef __cplusplus
}
#endif

#endif
