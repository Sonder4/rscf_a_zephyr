/*******************************************************************************

                      版权所有 (C), 2025-2026, NCU_Roboteam

 *******************************************************************************
  文 件 名: protocol.h
  版 本 号: 1.0.0
  作    者: Xuan
  生成日期: 2026-02-11
  功能描述: Zephyr 通信协议处理层头文件
  补    充: 协议版本 2.0.0

*******************************************************************************/

#ifndef __PROTOCOL_H
#define __PROTOCOL_H

#include <stdint.h>

#include "data_def.h"

#define PROTOCOL_HEAD1      0x52
#define PROTOCOL_HEAD2      0x43
#define PROTOCOL_TAIL       0xED

#define PROTOCOL_MAX_DATA_LEN   255
#define PROTOCOL_FRAME_MIN_LEN  10

typedef struct ProtocolFrame
{
    uint8_t header1;
    uint8_t header2;
    uint8_t heart;
    uint8_t mid;
    uint8_t pl;
    uint8_t* alldata_ptr;
    uint8_t pid;
    uint8_t pidLen;
    uint8_t* pidData_ptr;
    uint16_t crc;
    uint8_t tail;
    uint8_t crc_flag;
} ProtocolFrame_t;

void ProtocolFrame_Init(ProtocolFrame_t* frame);
uint16_t Protocol_Pack(uint8_t mid, uint8_t pid, const uint8_t* data, uint8_t len, uint8_t* out_buf);
uint16_t Protocol_Pack_Begin(uint8_t mid, uint8_t* out_buf);
uint16_t Protocol_Pack_Add(uint8_t pid, const uint8_t* data, uint8_t len, uint8_t* out_buf, uint16_t current_idx);
uint16_t Protocol_Pack_End(uint8_t* out_buf, uint16_t current_idx);
int8_t Protocol_Unpack(uint8_t byte, ProtocolFrame_t* frame);

#endif /* __PROTOCOL_H */
