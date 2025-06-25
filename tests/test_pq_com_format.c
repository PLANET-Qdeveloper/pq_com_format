#include "pq_com_format/pq_com_format.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_version() {
  assert(pq_com_format_version_number() == PQ_COM_FORMAT_VERSION);
  assert(strcmp(pq_com_format_version_string(), "1.0.0") == 0);
}

static void test_init_and_clear() {
  pq_com_format_t pkt;
  uint8_t payload[4] = {1, 2, 3, 4};
  assert(pq_com_format_init(&pkt, payload, 4) == PQ_COM_FORMAT_SUCCESS);
  assert(pkt.length == 4);
  assert(pkt.payload.size == 4);
  assert(memcmp(pkt.payload.data, payload, 4) == 0);
  assert(pkt.checksum == pq_com_format_calculate_checksum(payload, 4));
  assert(pq_com_format_clear(&pkt) == PQ_COM_FORMAT_SUCCESS);
  assert(pkt.length == 0);
  assert(pkt.payload.size == 0);
}

static void test_set() {
  pq_com_format_t pkt;
  pq_com_format_init(&pkt, NULL, 0);
  uint8_t payload[2] = {0xAA, 0xBB};
  assert(pq_com_format_set(&pkt, payload, 2) == PQ_COM_FORMAT_SUCCESS);
  assert(pkt.length == 2);
  assert(pkt.payload.size == 2);
  assert(memcmp(pkt.payload.data, payload, 2) == 0);
}

static void test_encode_decode() {
  pq_com_format_t pkt1, pkt2;
  uint8_t payload[3] = {9, 8, 7};
  pq_com_format_init(&pkt1, payload, 3);
  uint8_t buf[64];
  assert(pq_com_format_encode(&pkt1, buf, sizeof(buf)) ==
         PQ_COM_FORMAT_SUCCESS);
  assert(pq_com_format_decode(&pkt2, buf,
                              PQ_COM_FORMAT_HEADER_SIZE + 3 +
                                  PQ_COM_FORMAT_FOOTER_SIZE) ==
         PQ_COM_FORMAT_SUCCESS);
  assert(pkt2.length == 3);
  assert(memcmp(pkt2.payload.data, payload, 3) == 0);
  assert(pkt2.checksum == pkt1.checksum);
}

static void test_invalid_param() {
  assert(pq_com_format_init(NULL, NULL, 0) ==
         PQ_COM_FORMAT_ERROR_INVALID_PARAM);
  assert(pq_com_format_clear(NULL) == PQ_COM_FORMAT_ERROR_INVALID_PARAM);
  assert(pq_com_format_set(NULL, NULL, 0) == PQ_COM_FORMAT_ERROR_INVALID_PARAM);
  assert(pq_com_format_encode(NULL, NULL, 0) ==
         PQ_COM_FORMAT_ERROR_INVALID_PARAM);
  assert(pq_com_format_decode(NULL, NULL, 0) ==
         PQ_COM_FORMAT_ERROR_INVALID_PARAM);
}

static void test_buffer_api() {
  pq_com_format_buffer_t buf;
  // init
  pq_com_format_buffer_init(&buf);
  assert(buf.size == 0);
  assert(buf.capacity == PQ_COM_FORMAT_MAX_PACKET_SIZE);
  for (size_t i = 0; i < sizeof(buf.data); ++i) {
    assert(buf.data[i] == 0);
  }
  // clear
  buf.size = 10;
  pq_com_format_buffer_clear(&buf);
  assert(buf.size == 0);
  // ensure_capacity
  assert(pq_com_format_buffer_ensure_capacity(
             &buf, PQ_COM_FORMAT_MAX_PACKET_SIZE) == PQ_COM_FORMAT_SUCCESS);
  assert(pq_com_format_buffer_ensure_capacity(
             &buf, PQ_COM_FORMAT_MAX_PACKET_SIZE + 1) ==
         PQ_COM_FORMAT_ERROR_BUFFER_TOO_SMALL);
  assert(pq_com_format_buffer_ensure_capacity(NULL, 1) ==
         PQ_COM_FORMAT_ERROR_INVALID_PARAM);
}

static void test_result_string() {
  assert(strcmp(pq_com_format_result_string(PQ_COM_FORMAT_SUCCESS),
                "Success") == 0);
  assert(strcmp(pq_com_format_result_string(PQ_COM_FORMAT_ERROR_INVALID_PARAM),
                "Invalid parameter") == 0);
  assert(
      strcmp(pq_com_format_result_string(PQ_COM_FORMAT_ERROR_BUFFER_TOO_SMALL),
             "Buffer too small") == 0);
  assert(strcmp(pq_com_format_result_string(PQ_COM_FORMAT_ERROR_INVALID_FORMAT),
                "Invalid format") == 0);
  assert(strcmp(pq_com_format_result_string(PQ_COM_FORMAT_ERROR_MEMORY),
                "Memory error") == 0);
  assert(strcmp(pq_com_format_result_string(PQ_COM_FORMAT_ERROR_CHECKSUM),
                "Checksum error") == 0);
  assert(strcmp(pq_com_format_result_string((pq_com_format_result_t)999),
                "Unknown error") == 0);
}

static void test_size_overflow() {
  pq_com_format_t pkt;
  uint8_t big_payload[PQ_COM_FORMAT_MAX_PACKET_SIZE + 1];
  memset(big_payload, 0xAB, sizeof(big_payload));

  // pq_com_format_init: サイズオーバー
  assert(pq_com_format_init(&pkt, big_payload, sizeof(big_payload)) ==
         PQ_COM_FORMAT_ERROR_BUFFER_TOO_SMALL);

  // pq_com_format_set: サイズオーバー
  pq_com_format_init(&pkt, NULL, 0);
  assert(pq_com_format_set(&pkt, big_payload, sizeof(big_payload)) ==
         PQ_COM_FORMAT_ERROR_BUFFER_TOO_SMALL);

  // pq_com_format_encode: バッファ不足
  uint8_t payload[8] = {1, 2, 3, 4, 5, 6, 7, 8};
  pq_com_format_init(&pkt, payload, sizeof(payload));
  uint8_t small_buf[1];
  assert(pq_com_format_encode(&pkt, small_buf, sizeof(small_buf)) ==
         PQ_COM_FORMAT_ERROR_BUFFER_TOO_SMALL);

  // pq_com_format_decode: 入力サイズ不足
  uint8_t buf[64];
  pq_com_format_encode(&pkt, buf, sizeof(buf));
  pq_com_format_t pkt2;
  // 本来必要なサイズより1バイト小さい場合
  uint16_t required_size =
      PQ_COM_FORMAT_HEADER_SIZE + sizeof(payload) + PQ_COM_FORMAT_FOOTER_SIZE;
  assert(pq_com_format_decode(&pkt2, buf, required_size - 1) ==
         PQ_COM_FORMAT_ERROR_INVALID_FORMAT);
}

int main() {
  test_version();
  test_init_and_clear();
  test_set();
  test_encode_decode();
  test_invalid_param();
  test_buffer_api();
  test_result_string();
  test_size_overflow();
  printf("All tests passed!\n");
  return 0;
}
