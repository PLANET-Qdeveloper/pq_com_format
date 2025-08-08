#include "pq_com_format/pq_com_format.h"
#include "internal.h"
#include <string.h>

void pq_com_format_clear(pq_com_format_t *packet) {
  if (packet != NULL) {
    memset(packet, 0, sizeof(pq_com_format_t));
  }
}

uint16_t pq_com_format_calculate_crc_checksum(uint8_t *data, uint16_t length) {
  if (data == NULL) {
    return 0xFFFF;
  }
  
  uint16_t crc = 0xFFFF;
  for (uint16_t i = 0; i < length; i++) {
    crc ^= (uint16_t)data[i] << 8;
    for (uint8_t j = 0; j < 8U; j++) {
      if (crc & 0x8000U) {
        crc = (crc << 1) ^ 0x1021U;
      } else {
        crc <<= 1;
      }
    }
  }
  return crc;
}

/**
 * @brief デコードの状態
 */
typedef enum {
  DECODE_STATE_WAIT_START = 0,
  DECODE_STATE_DESTINATION_ID,
  DECODE_STATE_SOURCE_ID,
  DECODE_STATE_PAYLOAD_LENGTH,
  DECODE_STATE_PAYLOAD,
  DECODE_STATE_CRC_1,
  DECODE_STATE_CRC_2,
  DECODE_STATE_END
} decode_state_t;

static decode_state_t decode_state = DECODE_STATE_WAIT_START;
static uint8_t decode_is_stuffed = 0;

pq_com_format_decode_result_t pq_com_format_decode(
    pq_com_format_t *packet, const uint8_t input) {
  uint8_t current_byte = input;

  if (packet == NULL) {
    return PQ_COM_FORMAT_DECODE_ERROR_UNDEFINED;
  }

  if (decode_state == DECODE_STATE_WAIT_START) {
    if (input == PQ_COM_FORMAT_START_MARKER) {
      pq_com_format_clear(packet);
      decode_state = DECODE_STATE_DESTINATION_ID;
      decode_is_stuffed = 0;
      return PQ_COM_FORMAT_DECODE_VALID;
    }
    return PQ_COM_FORMAT_DECODE_ERROR_INVALID_HEADER;
  }

  if (decode_is_stuffed) {
    decode_is_stuffed = 0;
    switch (current_byte) {
    case 0x81:
      current_byte = 0x7E;
      break;
    case 0x80:
      current_byte = 0x7F;
      break;
    case 0x7D:
      current_byte = 0x7D;
      break;
    default:
      decode_state = DECODE_STATE_WAIT_START;
      return PQ_COM_FORMAT_DECODE_ERROR_UNDEFINED;
    }
  } else {
    if (current_byte == 0x7D) {
      decode_is_stuffed = 1;
      return PQ_COM_FORMAT_DECODE_VALID;
    }
  }

  if (current_byte == PQ_COM_FORMAT_END_MARKER) {
    if (decode_state != DECODE_STATE_END) {
      decode_state = DECODE_STATE_WAIT_START;
      return PQ_COM_FORMAT_DECODE_ERROR_INVALID_FOOTER;
    }
    uint16_t calculated_crc =
        pq_com_format_calculate_crc_checksum(packet->payload, packet->payload_length);
    decode_state = DECODE_STATE_WAIT_START;
    if (calculated_crc == packet->crc_checksum) {
      return PQ_COM_FORMAT_DECODE_COMPLETED;
    } else {
      return PQ_COM_FORMAT_DECODE_ERROR_INVALID_CHECKSUM;
    }
  }

  switch (decode_state) {
  case DECODE_STATE_DESTINATION_ID:
    packet->destination_id = current_byte;
    decode_state = DECODE_STATE_SOURCE_ID;
    break;
  case DECODE_STATE_SOURCE_ID:
    packet->source_id = current_byte;
    decode_state = DECODE_STATE_PAYLOAD_LENGTH;
    break;
  case DECODE_STATE_PAYLOAD_LENGTH:
    packet->payload_length = current_byte;
    if (packet->payload_length > PQ_COM_FORMAT_MAX_PAYLOAD_SIZE) {
      decode_state = DECODE_STATE_WAIT_START;
      return PQ_COM_FORMAT_DECODE_ERROR_UNDEFINED;
    }
    packet->index = 0;
    decode_state = (packet->payload_length == 0) ? DECODE_STATE_CRC_1
                                          : DECODE_STATE_PAYLOAD;
    break;
  case DECODE_STATE_PAYLOAD:
    packet->payload[packet->index++] = current_byte;
    if (packet->index >= packet->payload_length) {
      decode_state = DECODE_STATE_CRC_1;
    }
    break;
  case DECODE_STATE_CRC_1:
    packet->crc_checksum = (uint16_t)current_byte << 8;
    decode_state = DECODE_STATE_CRC_2;
    break;
  case DECODE_STATE_CRC_2:
    packet->crc_checksum |= current_byte;
    decode_state = DECODE_STATE_END;
    break;
  case DECODE_STATE_WAIT_START:
  case DECODE_STATE_END:
    // error
    decode_state = DECODE_STATE_WAIT_START;
    return PQ_COM_FORMAT_DECODE_ERROR_UNDEFINED;
  }

  return PQ_COM_FORMAT_DECODE_VALID;
}

static uint8_t encode_stuffed_byte = 0;
static uint8_t encode_stuffing_active = 0;

pq_com_format_encode_result_t pq_com_format_encode(
    pq_com_format_t *packet, uint8_t *output) {

  if (packet == NULL || output == NULL) {
    return PQ_COM_FORMAT_ENCODE_ERROR_UNDEFINED;
  }

  if (encode_stuffing_active) {
    *output = encode_stuffed_byte;
    encode_stuffing_active = 0;
    return PQ_COM_FORMAT_ENCODE_SUCCESS;
  }

  uint8_t byte_to_encode;
  const uint16_t payload_len = packet->payload_length;
  const uint16_t frame_len_without_markers = 3U + payload_len + 2U;

  if (packet->index == 0) {
    *output = PQ_COM_FORMAT_START_MARKER;
    packet->crc_checksum =
        pq_com_format_calculate_crc_checksum(packet->payload, payload_len);
    packet->index++;
    return PQ_COM_FORMAT_ENCODE_SUCCESS;
  }

  if (packet->index > frame_len_without_markers) {
    *output = PQ_COM_FORMAT_END_MARKER;
    packet->index = 0;
    return PQ_COM_FORMAT_ENCODE_COMPLETED;
  }

  const uint16_t current_pos = packet->index - 1U;
  if (current_pos == 0) {
    byte_to_encode = packet->destination_id;
  } else if (current_pos == 1) {
    byte_to_encode = packet->source_id;
  } else if (current_pos == 2) {
    byte_to_encode = packet->payload_length;
  } else if (current_pos < 3U + payload_len) {
    byte_to_encode = packet->payload[current_pos - 3U];
  } else if (current_pos == 3U + payload_len) {
    byte_to_encode = (uint8_t)(packet->crc_checksum >> 8);
  } else { // current_pos == 4 + payload_len
    byte_to_encode = (uint8_t)(packet->crc_checksum & 0xFF);
  }

  if (byte_to_encode == 0x7E || byte_to_encode == 0x7F ||
      byte_to_encode == 0x7D) {
    *output = 0x7D;
    encode_stuffing_active = 1;
    if (byte_to_encode == 0x7E)
      encode_stuffed_byte = 0x81;
    else if (byte_to_encode == 0x7F)
      encode_stuffed_byte = 0x80;
    else // 0x7D
      encode_stuffed_byte = 0x7D;
  } else {
    *output = byte_to_encode;
  }

  packet->index++;
  return PQ_COM_FORMAT_ENCODE_SUCCESS;
}

void pq_com_format_reset_decode_state(void) {
  decode_state = DECODE_STATE_WAIT_START;
  decode_is_stuffed = 0;
}

void pq_com_format_reset_encode_state(void) {
  encode_stuffed_byte = 0;
  encode_stuffing_active = 0;
}