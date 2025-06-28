#ifndef PQ_COM_FORMAT_H
#define PQ_COM_FORMAT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#define PQ_COM_FORMAT_MAX_PAYLOAD_SIZE 255
#define PQ_COM_FORMAT_MAX_PACKET_SIZE (PQ_COM_FORMAT_MAX_PAYLOAD_SIZE + 6)

#define PQ_COM_FORMAT_START_MARKER 0x7E
#define PQ_COM_FORMAT_END_MARKER 0x7F


/**
 * @enum pq_com_format_decode_result_t
 * @brief デコードの状況を示す
 */
typedef enum {
  /** 有効なデータをデコードした */
  PQ_COM_FORMAT_VALID = 0,
  /** データのデコードが完了 */
  PQ_COM_FORMAT_COMPLETED = 1,
  /** ヘッダが無効 */
  PQ_COM_FORMAT_ERROR_INVALID_HEADER = -1,
  /** フッタが無効 */
  PQ_COM_FORMAT_ERROR_INVALID_FOOTER = -2,
  /** チェックサムが無効 */
  PQ_COM_FORMAT_ERROR_INVALID_CHECKSUM = -3,
  /** 未定義のエラー */
  PQ_COM_FORMAT_ERROR_UNDEFINED = -255,
} pq_com_format_decode_result_t;

/**
 * @enum pq_com_format_encode_result_t
 * @brief エンコードの状況を示す
 */
typedef enum {
  /** 有効なデータをエンコードした */
  PQ_COM_FORMAT_ENCODE_SUCCESS = 0,
  /** エンコードが完了 */
  PQ_COM_FORMAT_ENCODE_COMPLETED = 1,
  /** 未定義のエラー */
  PQ_COM_FORMAT_ENCODE_ERROR_UNDEFINED = -255,
} pq_com_format_encode_result_t;

/**
 * @struct pq_com_format_t
 * @brief パケットのデータを格納する構造体  
 */
typedef struct {
  /** 宛先ID */
  uint8_t destination_id;
  /** 送信元ID */
  uint8_t source_id;
  /** ペイロード長(バイト数) */
  uint16_t payload_length;
  /** ペイロードデータ */
  uint8_t payload[PQ_COM_FORMAT_MAX_PAYLOAD_SIZE];
  /** ペイロードのCRCチェックサム */
  uint16_t crc_checksum;
  /** 現在の処理位置(バイト数) */
  uint16_t index;
} pq_com_format_t;


/** 
 * @fn
 * @brief パケットをクリアする
 * @param packet クリアするパケット
*/
void pq_com_format_clear(pq_com_format_t *packet);

/** 
 * @fn
 * @brief CRC16チェックサムを計算する
 * @param data チェックサムを計算するデータ
 * @param length チェックサムを計算するデータの長さ
 * @return CRC16チェックサム
 */
uint16_t pq_com_format_calculate_crc_checksum(uint8_t *data, uint16_t length);

/** 
 * @fn
 * @brief パケットにデコードしたいデータを1byteずつこの関数に渡す。順次データがpacketに格納されていく。完了するとPQ_COM_FORMAT_COMPLETED = 1を返す。
 * @param packet デコードするパケット
 * @param input デコードするデータ
 * @return デコード結果: 成功: PQ_COM_FORMAT_VALID = 0, 完了: PQ_COM_FORMAT_COMPLETED = 1
 * @see pq_com_format_decode_result_t
 */
pq_com_format_decode_result_t pq_com_format_decode(
    pq_com_format_t *packet, const uint8_t input);

/** 
 * @fn
 * @brief パケットをエンコードする。実行するたびにパケットのデータを1byteずつエンコードする。完了するとPQ_COM_FORMAT_ENCODE_COMPLETED = 1を返す。
 * @param packet エンコードするパケット
 * @param output エンコードされたデータ
 * @return エンコード結果: 成功: PQ_COM_FORMAT_ENCODE_SUCCESS = 0, 完了: PQ_COM_FORMAT_ENCODE_COMPLETED = 1
 */
pq_com_format_encode_result_t pq_com_format_encode(
    pq_com_format_t *packet, uint8_t *output);

#ifdef __cplusplus
}
#endif

#endif
