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
  PQ_COM_FORMAT_DECODE_VALID = 0,
  /** データのデコードが完了 */
  PQ_COM_FORMAT_DECODE_COMPLETED = 1,
  /** ヘッダが無効 */
  PQ_COM_FORMAT_DECODE_ERROR_INVALID_HEADER = -1,
  /** フッタが無効 */
  PQ_COM_FORMAT_DECODE_ERROR_INVALID_FOOTER = -2,
  /** チェックサムが無効 */
  PQ_COM_FORMAT_DECODE_ERROR_INVALID_CHECKSUM = -3,
  /** 未定義のエラー */
  PQ_COM_FORMAT_DECODE_ERROR_UNDEFINED = -255,
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
 * @enum pq_com_format_decode_state_t
 * @brief デコーダーの内部状態を示す
 */
typedef enum {
  /** スタートマーカーを待機中 */
  PQ_COM_FORMAT_DECODE_STATE_WAIT_START = 0,
  /** 宛先IDを受信中 */
  PQ_COM_FORMAT_DECODE_STATE_DESTINATION_ID,
  /** 送信元IDを受信中 */
  PQ_COM_FORMAT_DECODE_STATE_SOURCE_ID,
  /** ペイロード長を受信中 */
  PQ_COM_FORMAT_DECODE_STATE_PAYLOAD_LENGTH,
  /** ペイロードデータを受信中 */
  PQ_COM_FORMAT_DECODE_STATE_PAYLOAD,
  /** CRCチェックサムの上位バイトを受信中 */
  PQ_COM_FORMAT_DECODE_STATE_CRC_1,
  /** CRCチェックサムの下位バイトを受信中 */
  PQ_COM_FORMAT_DECODE_STATE_CRC_2,
  /** エンドマーカーを待機中 */
  PQ_COM_FORMAT_DECODE_STATE_END,
} pq_com_format_decode_state_t;

/**
 * @struct pq_com_format_t
 * @brief パケットのデータとエンコード/デコード状態を格納する構造体
 * 
 * この構造体はパケットのデータだけでなく、エンコード・デコード処理の
 * 内部状態も保持します。これにより、関数を完全にステートレスにして
 * 複数のパケットを並行して処理できるようになります。
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
  
  /* デコード用の状態変数 */
  /** デコーダーの現在の状態 */
  pq_com_format_decode_state_t decode_state;
  /** バイトスタッフィングが適用された次のバイトかどうか */
  uint8_t is_stuffed;
  
  /* エンコード用の状態変数 */
  /** スタッフィングされたバイト */
  uint8_t stuffed_byte;
  /** スタッフィング処理が有効かどうか */
  uint8_t stuffing_active;
} pq_com_format_t;


/** 
 * @fn pq_com_format_clear
 * @brief パケット構造体を初期化し、全ての状態をクリアする
 * 
 * この関数は pq_com_format_t 構造体の全メンバーを0で初期化します。
 * パケットの使用開始前、またはリセットが必要な際に呼び出してください。
 * 
 * @param packet 初期化するパケット構造体へのポインタ
 */
void pq_com_format_clear(pq_com_format_t *packet);

/** 
 * @fn pq_com_format_calculate_crc_checksum
 * @brief CRC16チェックサムを計算する（CRC-CCITT多項式使用）
 * 
 * この関数はCRC-CCITT多項式（0x1021）を使用してCRC16チェックサムを計算します。
 * 初期値は0xFFFFです。主にペイロードデータの整合性検証に使用されます。
 * 
 * @param data チェックサムを計算するデータバッファへのポインタ
 * @param length チェックサムを計算するデータの長さ（バイト数）
 * @return 計算されたCRC16チェックサム値
 */
uint16_t pq_com_format_calculate_crc_checksum(uint8_t *data, uint16_t length);

/** 
 * @fn pq_com_format_decode
 * @brief 受信したバイトデータを段階的にデコードしてパケットを構築する
 * 
 * この関数は受信データを1バイトずつ処理し、パケットの各フィールドを順次デコードします。
 * バイトスタッフィング（0x7D, 0x7E, 0x7Fのエスケープ処理）にも対応しています。
 * 完全なパケットが受信されるまで繰り返し呼び出してください。
 * 
 * デコード状態は packet->decode_state に保存されるため、複数のパケットを
 * 並行してデコードすることが可能です。
 * 
 * @param packet デコード状態とデータを格納するパケット構造体へのポインタ
 * @param input 処理する受信バイトデータ
 * @return デコード結果
 *         - PQ_COM_FORMAT_DECODE_VALID(0): 正常に処理されたが、まだ完了していない
 *         - PQ_COM_FORMAT_DECODE_COMPLETED(1): パケットのデコードが完了した
 *         - 負の値: エラーが発生した（詳細は pq_com_format_decode_result_t を参照）
 * @see pq_com_format_decode_result_t
 */
pq_com_format_decode_result_t pq_com_format_decode(
    pq_com_format_t *packet, const uint8_t input);

/** 
 * @fn pq_com_format_encode
 * @brief パケットデータを段階的にエンコードして送信用バイトストリームを生成する
 * 
 * この関数はパケット構造体の内容を1バイトずつエンコードし、送信可能な
 * バイトストリームに変換します。バイトスタッフィング処理も自動的に行います。
 * 完全なパケットがエンコードされるまで繰り返し呼び出してください。
 * 
 * エンコード状態は packet->index, packet->stuffing_active, packet->stuffed_byte に
 * 保存されるため、複数のパケットを並行してエンコードすることが可能です。
 * 
 * 最初の呼び出し時に自動的にCRCチェックサムが計算されます。
 * 
 * @param packet エンコードするパケット構造体へのポインタ
 * @param output エンコードされたバイトデータを格納するバッファへのポインタ
 * @return エンコード結果
 *         - PQ_COM_FORMAT_ENCODE_SUCCESS(0): 1バイト正常にエンコードされた
 *         - PQ_COM_FORMAT_ENCODE_COMPLETED(1): パケットのエンコードが完了した
 *         - 負の値: エラーが発生した（詳細は pq_com_format_encode_result_t を参照）
 */
pq_com_format_encode_result_t pq_com_format_encode(
    pq_com_format_t *packet, uint8_t *output);

#ifdef __cplusplus
}
#endif

#endif
