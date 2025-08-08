#include "pq_com_format/pq_com_format.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

void test_packet_clear(void) {
    printf("Testing packet clear...\n");
    pq_com_format_t packet;
    packet.destination_id = 0x12;
    packet.source_id = 0x34;
    packet.payload_length = 10;
    packet.crc_checksum = 0xABCD;
    packet.index = 5;
    
    pq_com_format_clear(&packet);
    
    assert(packet.destination_id == 0);
    assert(packet.source_id == 0);
    assert(packet.payload_length == 0);
    assert(packet.crc_checksum == 0);
    assert(packet.index == 0);
    printf("  ✓ Packet clear test passed\n");
}

void test_crc_calculation(void) {
    printf("Testing CRC calculation...\n");
    uint8_t test_data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    uint16_t crc = pq_com_format_calculate_crc_checksum(test_data, 5);
    assert(crc != 0);
    
    uint8_t empty_data[] = {};
    uint16_t empty_crc = pq_com_format_calculate_crc_checksum(empty_data, 0);
    assert(empty_crc == 0xFFFF);
    
    printf("  ✓ CRC calculation test passed (CRC: 0x%04X)\n", crc);
}

void test_basic_encode_decode(void) {
    printf("Testing basic encode/decode...\n");
    pq_com_format_t tx_packet, rx_packet;
    
    pq_com_format_clear(&tx_packet);
    tx_packet.destination_id = 0x12;
    tx_packet.source_id = 0x34;
    tx_packet.payload_length = 5;
    memcpy(tx_packet.payload, "Hello", 5);
    
    uint8_t encoded_buffer[256];
    uint8_t *buffer_ptr = encoded_buffer;
    pq_com_format_encode_result_t encode_result;
    
    do {
        encode_result = pq_com_format_encode(&tx_packet, buffer_ptr++);
        assert(buffer_ptr - encoded_buffer < 256);
    } while (encode_result == PQ_COM_FORMAT_ENCODE_SUCCESS);
    
    assert(encode_result == PQ_COM_FORMAT_ENCODE_COMPLETED);
    size_t encoded_length = buffer_ptr - encoded_buffer;
    
    pq_com_format_clear(&rx_packet);
    pq_com_format_decode_result_t decode_result;
    
    for (size_t i = 0; i < encoded_length; i++) {
        decode_result = pq_com_format_decode(&rx_packet, encoded_buffer[i]);
        if (i < encoded_length - 1) {
            assert(decode_result == PQ_COM_FORMAT_DECODE_VALID);
        }
    }
    
    assert(decode_result == PQ_COM_FORMAT_DECODE_COMPLETED);
    assert(rx_packet.destination_id == tx_packet.destination_id);
    assert(rx_packet.source_id == tx_packet.source_id);
    assert(rx_packet.payload_length == tx_packet.payload_length);
    assert(memcmp(rx_packet.payload, tx_packet.payload, tx_packet.payload_length) == 0);
    
    printf("  ✓ Basic encode/decode test passed (encoded %zu bytes)\n", encoded_length);
}

void test_empty_payload(void) {
    printf("Testing empty payload...\n");
    pq_com_format_t tx_packet, rx_packet;
    
    pq_com_format_clear(&tx_packet);
    tx_packet.destination_id = 0xAA;
    tx_packet.source_id = 0xBB;
    tx_packet.payload_length = 0;
    
    uint8_t encoded_buffer[256];
    uint8_t *buffer_ptr = encoded_buffer;
    pq_com_format_encode_result_t encode_result;
    
    do {
        encode_result = pq_com_format_encode(&tx_packet, buffer_ptr++);
    } while (encode_result == PQ_COM_FORMAT_ENCODE_SUCCESS);
    
    assert(encode_result == PQ_COM_FORMAT_ENCODE_COMPLETED);
    size_t encoded_length = buffer_ptr - encoded_buffer;
    
    pq_com_format_clear(&rx_packet);
    for (size_t i = 0; i < encoded_length; i++) {
        pq_com_format_decode_result_t decode_result = pq_com_format_decode(&rx_packet, encoded_buffer[i]);
        if (i == encoded_length - 1) {
            assert(decode_result == PQ_COM_FORMAT_DECODE_COMPLETED);
        }
    }
    
    assert(rx_packet.destination_id == 0xAA);
    assert(rx_packet.source_id == 0xBB);
    assert(rx_packet.payload_length == 0);
    
    printf("  ✓ Empty payload test passed\n");
}

void test_max_payload(void) {
    printf("Testing maximum payload...\n");
    pq_com_format_t tx_packet, rx_packet;
    
    pq_com_format_clear(&tx_packet);
    tx_packet.destination_id = 0x01;
    tx_packet.source_id = 0x02;
    tx_packet.payload_length = PQ_COM_FORMAT_MAX_PAYLOAD_SIZE;
    
    for (int i = 0; i < PQ_COM_FORMAT_MAX_PAYLOAD_SIZE; i++) {
        tx_packet.payload[i] = (uint8_t)(i & 0xFF);
    }
    
    uint8_t encoded_buffer[512];
    uint8_t *buffer_ptr = encoded_buffer;
    pq_com_format_encode_result_t encode_result;
    
    do {
        encode_result = pq_com_format_encode(&tx_packet, buffer_ptr++);
        assert(buffer_ptr - encoded_buffer < 512);
    } while (encode_result == PQ_COM_FORMAT_ENCODE_SUCCESS);
    
    assert(encode_result == PQ_COM_FORMAT_ENCODE_COMPLETED);
    size_t encoded_length = buffer_ptr - encoded_buffer;
    
    pq_com_format_clear(&rx_packet);
    for (size_t i = 0; i < encoded_length; i++) {
        pq_com_format_decode_result_t decode_result = pq_com_format_decode(&rx_packet, encoded_buffer[i]);
        if (i == encoded_length - 1) {
            assert(decode_result == PQ_COM_FORMAT_DECODE_COMPLETED);
        }
    }
    
    assert(rx_packet.payload_length == PQ_COM_FORMAT_MAX_PAYLOAD_SIZE);
    assert(memcmp(rx_packet.payload, tx_packet.payload, PQ_COM_FORMAT_MAX_PAYLOAD_SIZE) == 0);
    
    printf("  ✓ Maximum payload test passed\n");
}

void test_byte_stuffing(void) {
    printf("Testing byte stuffing...\n");
    pq_com_format_t tx_packet, rx_packet;
    
    pq_com_format_clear(&tx_packet);
    tx_packet.destination_id = 0x7E;
    tx_packet.source_id = 0x7F;
    tx_packet.payload_length = 3;
    tx_packet.payload[0] = 0x7D;
    tx_packet.payload[1] = 0x7E;
    tx_packet.payload[2] = 0x7F;
    
    uint8_t encoded_buffer[256];
    uint8_t *buffer_ptr = encoded_buffer;
    pq_com_format_encode_result_t encode_result;
    
    do {
        encode_result = pq_com_format_encode(&tx_packet, buffer_ptr++);
    } while (encode_result == PQ_COM_FORMAT_ENCODE_SUCCESS);
    
    assert(encode_result == PQ_COM_FORMAT_ENCODE_COMPLETED);
    size_t encoded_length = buffer_ptr - encoded_buffer;
    
    int stuff_count = 0;
    for (size_t i = 1; i < encoded_length - 1; i++) {
        if (encoded_buffer[i] == 0x7D) stuff_count++;
    }
    assert(stuff_count > 0);
    
    pq_com_format_clear(&rx_packet);
    for (size_t i = 0; i < encoded_length; i++) {
        pq_com_format_decode_result_t decode_result = pq_com_format_decode(&rx_packet, encoded_buffer[i]);
        if (i == encoded_length - 1) {
            assert(decode_result == PQ_COM_FORMAT_DECODE_COMPLETED);
        }
    }
    
    assert(rx_packet.destination_id == 0x7E);
    assert(rx_packet.source_id == 0x7F);
    assert(rx_packet.payload_length == 3);
    assert(rx_packet.payload[0] == 0x7D);
    assert(rx_packet.payload[1] == 0x7E);
    assert(rx_packet.payload[2] == 0x7F);
    
    printf("  ✓ Byte stuffing test passed (found %d stuff bytes)\n", stuff_count);
}

void test_invalid_header(void) {
    printf("Testing invalid header handling...\n");
    pq_com_format_t packet;
    pq_com_format_clear(&packet);
    
    pq_com_format_decode_result_t result = pq_com_format_decode(&packet, 0x12);
    assert(result == PQ_COM_FORMAT_DECODE_ERROR_INVALID_HEADER);
    
    result = pq_com_format_decode(&packet, 0x7E);
    assert(result == PQ_COM_FORMAT_DECODE_VALID);
    
    printf("  ✓ Invalid header test passed\n");
}

void test_invalid_checksum(void) {
    printf("Testing invalid checksum handling...\n");
    pq_com_format_t packet;
    pq_com_format_clear(&packet);
    
    uint8_t invalid_packet[] = {0x7E, 0x01, 0x02, 0x01, 0xFF, 0x00, 0x00, 0x7F};
    
    pq_com_format_decode_result_t result;
    for (size_t i = 0; i < sizeof(invalid_packet); i++) {
        result = pq_com_format_decode(&packet, invalid_packet[i]);
    }
    
    assert(result == PQ_COM_FORMAT_DECODE_ERROR_INVALID_CHECKSUM);
    
    printf("  ✓ Invalid checksum test passed\n");
}

void test_state_reset_functions(void) {
    printf("Testing state reset functions...\n");
    pq_com_format_t packet;
    pq_com_format_clear(&packet);
    
    pq_com_format_decode(&packet, 0x7E);
    pq_com_format_decode(&packet, 0x01);
    
    pq_com_format_reset_decode_state();
    
    pq_com_format_decode_result_t result = pq_com_format_decode(&packet, 0x12);
    assert(result == PQ_COM_FORMAT_DECODE_ERROR_INVALID_HEADER);
    
    pq_com_format_reset_encode_state();
    
    printf("  ✓ State reset functions test passed\n");
}

void test_null_pointer_handling(void) {
    printf("Testing null pointer handling...\n");
    uint8_t output;
    
    pq_com_format_decode_result_t decode_result = pq_com_format_decode(NULL, 0x7E);
    assert(decode_result == PQ_COM_FORMAT_DECODE_ERROR_UNDEFINED);
    
    pq_com_format_encode_result_t encode_result = pq_com_format_encode(NULL, &output);
    assert(encode_result == PQ_COM_FORMAT_ENCODE_ERROR_UNDEFINED);
    
    pq_com_format_t packet;
    encode_result = pq_com_format_encode(&packet, NULL);
    assert(encode_result == PQ_COM_FORMAT_ENCODE_ERROR_UNDEFINED);
    
    uint16_t crc = pq_com_format_calculate_crc_checksum(NULL, 5);
    assert(crc == 0xFFFF);
    
    pq_com_format_clear(NULL);
    
    printf("  ✓ Null pointer handling test passed\n");
}

void test_oversized_payload_length(void) {
    printf("Testing payload length validation...\n");
    pq_com_format_t packet;
    pq_com_format_clear(&packet);
    
    pq_com_format_decode(&packet, 0x7E);
    pq_com_format_decode(&packet, 0x01);
    pq_com_format_decode(&packet, 0x02);
    pq_com_format_decode_result_t result = pq_com_format_decode(&packet, 0xFF);
    
    assert(result == PQ_COM_FORMAT_DECODE_VALID);
    assert(packet.payload_length == 0xFF);
    
    printf("  ✓ Payload length validation test passed\n");
}

int main(void) {
    printf("=== PQ Communication Format Test Suite ===\n\n");
    
    test_packet_clear();
    test_crc_calculation();
    test_basic_encode_decode();
    test_empty_payload();
    test_max_payload();
    test_byte_stuffing();
    test_invalid_header();
    test_invalid_checksum();
    test_state_reset_functions();
    test_null_pointer_handling();
    test_oversized_payload_length();
    
    printf("\n=== All tests passed! ===\n");
    return 0;
}
