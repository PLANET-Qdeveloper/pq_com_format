#include "pq_com_format/pq_com_format.h"
#include <string.h>

/**
 * @brief パケット構造体を初期化し、全ての状態をクリアする
 * 
 * パケットのデータフィールドとエンコード/デコード状態の両方を
 * 初期化します。新しいパケット処理を開始する前に必ず呼び出してください。
 */
void pq_com_format_clear(pq_com_format_t *packet) {
  memset(packet, 0, sizeof(pq_com_format_t));
  packet->decode_state = PQ_COM_FORMAT_DECODE_STATE_WAIT_START;
}

/**
 * @brief CRC16チェックサムを計算する（CRC-CCITT多項式使用）
 * 
 * CRC-CCITT多項式（0x1021）を使用してCRC16チェックサムを計算します。
 * 初期値は0xFFFFで、MSBファーストで処理されます。
 * 
 * @param data チェックサム計算対象のデータバッファ
 * @param length データ長（バイト数）
 * @return 計算されたCRC16値
 */
uint16_t pq_com_format_calculate_crc_checksum(uint8_t *data, uint16_t length) {
  uint16_t crc = 0xFFFF;
  for (uint16_t i = 0; i < length; i++) {
    crc ^= (uint16_t)data[i] << 8;
    for (uint8_t j = 0; j < 8; j++) {
      if (crc & 0x8000) {
        crc = (crc << 1) ^ 0x1021;
      } else {
        crc <<= 1;
      }
    }
  }
  return crc;
}


/**
 * @brief 受信したバイトデータを段階的にデコードしてパケットを構築する
 * 
 * この関数は完全にステートレスで、全ての状態情報はpacket構造体に保存されます。
 * バイトスタッフィング処理とパケット形式の検証を行います。
 * 
 * @param packet デコード状態とデータを格納するパケット構造体
 * @param input 処理する受信バイト
 * @return デコード結果（成功、完了、またはエラー）
 */
pq_com_format_decode_result_t pq_com_format_decode(
    pq_com_format_t *packet, const uint8_t input) {
  uint8_t current_byte = input;

  /* スタートマーカー待ち状態での処理 */
  if (packet->decode_state == PQ_COM_FORMAT_DECODE_STATE_WAIT_START) {
    if (input == PQ_COM_FORMAT_START_MARKER) {
      pq_com_format_clear(packet);
      packet->decode_state = PQ_COM_FORMAT_DECODE_STATE_DESTINATION_ID;
      return PQ_COM_FORMAT_DECODE_VALID;
    }
    return PQ_COM_FORMAT_DECODE_ERROR_INVALID_HEADER;
  }

  /* バイトスタッフィング処理 */
  if (packet->is_stuffed) {
    packet->is_stuffed = 0;
    switch (current_byte) {
    case 0x81:  /* スタッフィングされた0x7E */
      current_byte = 0x7E;
      break;
    case 0x80:  /* スタッフィングされた0x7F */
      current_byte = 0x7F;
      break;
    case 0x7D:  /* スタッフィングされた0x7D */
      current_byte = 0x7D;
      break;
    default:
      /* 無効なスタッフィングシーケンス */
      packet->decode_state = PQ_COM_FORMAT_DECODE_STATE_WAIT_START;
      return PQ_COM_FORMAT_DECODE_ERROR_UNDEFINED;
    }
  } else {
    if (current_byte == 0x7D) {
      /* スタッフィングエスケープ文字を検出 */
      packet->is_stuffed = 1;
      return PQ_COM_FORMAT_DECODE_VALID;
    }
  }

  /* エンドマーカーの処理 */
  if (current_byte == PQ_COM_FORMAT_END_MARKER) {
    if (packet->decode_state != PQ_COM_FORMAT_DECODE_STATE_END) {
      packet->decode_state = PQ_COM_FORMAT_DECODE_STATE_WAIT_START;
      return PQ_COM_FORMAT_DECODE_ERROR_INVALID_FOOTER;
    }
    /* CRCチェックサムの検証 */
    uint16_t calculated_crc =
        pq_com_format_calculate_crc_checksum(packet->payload, packet->payload_length);
    packet->decode_state = PQ_COM_FORMAT_DECODE_STATE_WAIT_START;
    if (calculated_crc == packet->crc_checksum) {
      return PQ_COM_FORMAT_DECODE_COMPLETED;
    } else {
      return PQ_COM_FORMAT_DECODE_ERROR_INVALID_CHECKSUM;
    }
  }

  /* 各フィールドの処理 */
  switch (packet->decode_state) {
  case PQ_COM_FORMAT_DECODE_STATE_DESTINATION_ID:
    packet->destination_id = current_byte;
    packet->decode_state = PQ_COM_FORMAT_DECODE_STATE_SOURCE_ID;
    break;
  case PQ_COM_FORMAT_DECODE_STATE_SOURCE_ID:
    packet->source_id = current_byte;
    packet->decode_state = PQ_COM_FORMAT_DECODE_STATE_PAYLOAD_LENGTH;
    break;
  case PQ_COM_FORMAT_DECODE_STATE_PAYLOAD_LENGTH:
    packet->payload_length = current_byte;
    if (packet->payload_length > PQ_COM_FORMAT_MAX_PAYLOAD_SIZE) {
      packet->decode_state = PQ_COM_FORMAT_DECODE_STATE_WAIT_START;
      return PQ_COM_FORMAT_DECODE_ERROR_UNDEFINED;
    }
    packet->index = 0;
    packet->decode_state = (packet->payload_length == 0) ? PQ_COM_FORMAT_DECODE_STATE_CRC_1
                                          : PQ_COM_FORMAT_DECODE_STATE_PAYLOAD;
    break;
  case PQ_COM_FORMAT_DECODE_STATE_PAYLOAD:
    packet->payload[packet->index++] = current_byte;
    if (packet->index >= packet->payload_length) {
      packet->decode_state = PQ_COM_FORMAT_DECODE_STATE_CRC_1;
    }
    break;
  case PQ_COM_FORMAT_DECODE_STATE_CRC_1:
    packet->crc_checksum = (uint16_t)current_byte << 8;
    packet->decode_state = PQ_COM_FORMAT_DECODE_STATE_CRC_2;
    break;
  case PQ_COM_FORMAT_DECODE_STATE_CRC_2:
    packet->crc_checksum |= current_byte;
    packet->decode_state = PQ_COM_FORMAT_DECODE_STATE_END;
    break;
  case PQ_COM_FORMAT_DECODE_STATE_WAIT_START:
  case PQ_COM_FORMAT_DECODE_STATE_END:
    /* エラー状態 */
    packet->decode_state = PQ_COM_FORMAT_DECODE_STATE_WAIT_START;
    return PQ_COM_FORMAT_DECODE_ERROR_UNDEFINED;
  }

  return PQ_COM_FORMAT_DECODE_VALID;
}

/**
 * @brief パケットデータを段階的にエンコードして送信用バイトストリームを生成する
 * 
 * この関数は完全にステートレスで、全ての状態情報はpacket構造体に保存されます。
 * バイトスタッフィング処理と送信フォーマットの生成を行います。
 * 
 * @param packet エンコードするパケット構造体
 * @param output エンコードされたバイトを格納するバッファ
 * @return エンコード結果（成功、完了、またはエラー）
 */
pq_com_format_encode_result_t pq_com_format_encode(
    pq_com_format_t *packet, uint8_t *output) {
  
  /* スタッフィング処理が有効な場合、スタッフィングされたバイトを出力 */
  if (packet->stuffing_active) {
    *output = packet->stuffed_byte;
    packet->stuffing_active = 0;
    return PQ_COM_FORMAT_ENCODE_SUCCESS;
  }

  uint8_t byte_to_encode;
  uint16_t payload_len = packet->payload_length;
  uint16_t frame_len_without_markers = 3 + payload_len + 2;  /* dest_id + src_id + len + payload + crc(2) */

  /* 最初の呼び出し時：スタートマーカーを出力し、CRCを計算 */
  if (packet->index == 0) {
    *output = PQ_COM_FORMAT_START_MARKER;
    packet->crc_checksum =
        pq_com_format_calculate_crc_checksum(packet->payload, payload_len);
    packet->index++;
    return PQ_COM_FORMAT_ENCODE_SUCCESS;
  }

  /* 全データ送信完了時：エンドマーカーを出力してリセット */
  if (packet->index > frame_len_without_markers) {
    *output = PQ_COM_FORMAT_END_MARKER;
    packet->index = 0;
    return PQ_COM_FORMAT_ENCODE_COMPLETED;
  }

  /* エンコード対象バイトの選択（現在位置に基づく） */
  uint16_t current_pos = packet->index - 1;
  if (current_pos == 0) {
    /* 宛先ID */
    byte_to_encode = packet->destination_id;
  } else if (current_pos == 1) {
    /* 送信元ID */
    byte_to_encode = packet->source_id;
  } else if (current_pos == 2) {
    /* ペイロード長 */
    byte_to_encode = packet->payload_length;
  } else if (current_pos < 3 + payload_len) {
    /* ペイロードデータ */
    byte_to_encode = packet->payload[current_pos - 3];
  } else if (current_pos == 3 + payload_len) {
    /* CRCチェックサムの上位バイト */
    byte_to_encode = (uint8_t)(packet->crc_checksum >> 8);
  } else { 
    /* CRCチェックサムの下位バイト (current_pos == 4 + payload_len) */
    byte_to_encode = (uint8_t)(packet->crc_checksum & 0xFF);
  }

  /* バイトスタッフィング処理 */
  if (byte_to_encode == 0x7E || byte_to_encode == 0x7F || byte_to_encode == 0x7D) {
    /* エスケープ文字を出力し、次回呼び出しでスタッフィングされたバイトを出力 */
    *output = 0x7D;
    packet->stuffing_active = 1;
    if (byte_to_encode == 0x7E)
      packet->stuffed_byte = 0x81;
    else if (byte_to_encode == 0x7F)
      packet->stuffed_byte = 0x80;
    else  /* 0x7D */
      packet->stuffed_byte = 0x7D;
  } else {
    /* 通常のバイトをそのまま出力 */
    *output = byte_to_encode;
  }

  packet->index++;
  return PQ_COM_FORMAT_ENCODE_SUCCESS;
}