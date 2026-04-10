#ifndef _BSP_LOG_H
#define _BSP_LOG_H

#include "SEGGER_RTT.h"
#include <stdint.h>
#include <stdio.h>

#ifndef DISABLE_LOG_SYSTEM
#define DISABLE_LOG_SYSTEM 0
#endif

typedef enum
{
    BSP_LOG_CHANNEL_BOOT = 0,
    BSP_LOG_CHANNEL_DIAG = 1,
    BSP_LOG_CHANNEL_RUNTIME = 2,
    BSP_LOG_CHANNEL_PLOT = 3,
    BSP_LOG_CHANNEL_COUNT
} BSPLogChannel;

typedef enum
{
    BSP_LOG_DIAG_LEVEL_INFO = 'I',
    BSP_LOG_DIAG_LEVEL_WARNING = 'W',
    BSP_LOG_DIAG_LEVEL_ERROR = 'E',
    BSP_LOG_DIAG_LEVEL_FATAL = 'F'
} BSPLogDiagLevel;

#define BSP_LOG_BINARY_MAGIC    0xBDFDU
#define BSP_LOG_BINARY_VERSION  1U

typedef enum
{
    BSP_LOG_BINARY_FRAME_KIND_META_DICT = 1,
    BSP_LOG_BINARY_FRAME_KIND_META_SCHEMA = 2,
    BSP_LOG_BINARY_FRAME_KIND_DIAG_DATA = 3,
    BSP_LOG_BINARY_FRAME_KIND_PLOT_DATA = 4,
    BSP_LOG_BINARY_FRAME_KIND_STATS = 5,
    BSP_LOG_BINARY_FRAME_KIND_HEARTBEAT = 6
} BSPLogBinaryFrameKind;

typedef enum
{
    BSP_LOG_BINARY_DICT_KIND_MODULE = 1,
    BSP_LOG_BINARY_DICT_KIND_EVENT = 2,
    BSP_LOG_BINARY_DICT_KIND_STREAM = 3,
    BSP_LOG_BINARY_DICT_KIND_FIELD = 4
} BSPLogBinaryDictKind;

typedef enum
{
    BSP_LOG_BINARY_SCHEMA_KIND_DIAG = 1,
    BSP_LOG_BINARY_SCHEMA_KIND_PLOT = 2
} BSPLogBinarySchemaKind;

typedef enum
{
    BSP_LOG_BINARY_VALUE_TYPE_I32 = 1
} BSPLogBinaryValueType;

typedef struct __attribute__((packed))
{
    uint16_t magic;
    uint8_t version;
    uint8_t kind;
    uint16_t payload_len;
    uint16_t flags;
    uint16_t seq;
    uint16_t tick_ms;
} BSPLogBinaryFrameHeader;

unsigned int BSPLogChannelToRtt(BSPLogChannel channel);
void BSPLogInit(void);
void BSPLogResetUpBuffer(void);
void BSPLogResetChannel(BSPLogChannel channel);
void BSPLogPlotSetEnabled(uint8_t enabled);
uint8_t BSPLogPlotIsEnabled(void);
void BSPLogBinaryEmitDict(BSPLogChannel channel, uint8_t dict_kind, uint8_t entry_id, const char *name, uint16_t seq, uint16_t tick_ms);
void BSPLogBinaryEmitSchema(
    BSPLogChannel channel,
    uint8_t schema_id,
    uint8_t schema_kind,
    uint8_t value_type,
    int8_t scale_pow10,
    const uint8_t *field_ids,
    uint8_t field_count,
    uint16_t seq,
    uint16_t tick_ms);
void BSPLogBinaryEmitDiagData(
    BSPLogChannel channel,
    uint8_t module_id,
    uint8_t event_id,
    uint8_t schema_id,
    const int32_t *values,
    uint8_t value_count,
    uint16_t seq,
    uint16_t tick_ms,
    uint16_t flags);
void BSPLogBinaryEmitPlotData(
    BSPLogChannel channel,
    uint8_t stream_id,
    uint8_t value_type,
    int8_t scale_pow10,
    const int32_t *values,
    uint8_t value_count,
    uint16_t seq,
    uint16_t tick_ms,
    uint16_t flags);
void BSPLogPlotF32(const char *stream, const float *values, uint32_t count);
void BSPLogPlotBenchFrame(const char *stream, uint32_t tick_ms, uint32_t seq, uint32_t channel_mask, uint32_t payload_width);
void BSPLogDiagEvent(BSPLogDiagLevel level, const char *module, const char *event, const char *kv_pairs);
void BSPLogDiagBenchFrame(uint32_t tick_ms, uint32_t seq, uint32_t channel_mask, uint32_t payload_width);
void BSPLogDiagMeta(const char *key, const char *value);
void BSPLogDiagCounter(const char *name, uint32_t value);
void BSPLogRuntimeBenchFrame(uint32_t tick_ms, uint32_t seq, uint32_t channel_mask, uint32_t payload_width);
void BSPLogGetChannelStats(BSPLogChannel channel, uint32_t *write_count, uint32_t *drop_count, uint32_t *seq);
int BSPLogPrintf(BSPLogChannel channel, const char *fmt, ...);

#define LOG_CHANNEL_PROTO(channel, type, color, format, ...) \
        BSPLogPrintf((channel), "  %s%s" format "\r\n%s",    \
                     color,                                   \
                     type,                                    \
                     ##__VA_ARGS__,                           \
                     RTT_CTRL_RESET)

#define LOG_CLEAR() SEGGER_RTT_WriteString(BSPLogChannelToRtt(BSP_LOG_CHANNEL_RUNTIME), "  " RTT_CTRL_CLEAR)
#define LOG(format, ...) LOG_CHANNEL_PROTO(BSP_LOG_CHANNEL_RUNTIME, "", "", format, ##__VA_ARGS__)
#define LOGBOOT(format, ...) LOG_CHANNEL_PROTO(BSP_LOG_CHANNEL_BOOT, "", "", format, ##__VA_ARGS__)
#define LOGDIAG(format, ...) LOG_CHANNEL_PROTO(BSP_LOG_CHANNEL_DIAG, "", "", format, ##__VA_ARGS__)

#if DISABLE_LOG_SYSTEM
#define LOGINFO(format, ...)
#define LOGWARNING(format, ...)
#define LOGERROR(format, ...)
#define LOGBOOTINFO(format, ...)
#define LOGBOOTWARNING(format, ...)
#define LOGBOOTERROR(format, ...)
#define LOGDIAGINFO(format, ...)
#define LOGDIAGWARNING(format, ...)
#define LOGDIAGERROR(format, ...)
#else
#define LOGINFO(format, ...) LOG_CHANNEL_PROTO(BSP_LOG_CHANNEL_RUNTIME, "I:", RTT_CTRL_TEXT_BRIGHT_GREEN, format, ##__VA_ARGS__)
#define LOGWARNING(format, ...) LOG_CHANNEL_PROTO(BSP_LOG_CHANNEL_RUNTIME, "W:", RTT_CTRL_TEXT_BRIGHT_YELLOW, format, ##__VA_ARGS__)
#define LOGERROR(format, ...) LOG_CHANNEL_PROTO(BSP_LOG_CHANNEL_RUNTIME, "E:", RTT_CTRL_TEXT_BRIGHT_RED, format, ##__VA_ARGS__)
#define LOGBOOTINFO(format, ...) LOG_CHANNEL_PROTO(BSP_LOG_CHANNEL_BOOT, "I:", RTT_CTRL_TEXT_BRIGHT_GREEN, format, ##__VA_ARGS__)
#define LOGBOOTWARNING(format, ...) LOG_CHANNEL_PROTO(BSP_LOG_CHANNEL_BOOT, "W:", RTT_CTRL_TEXT_BRIGHT_YELLOW, format, ##__VA_ARGS__)
#define LOGBOOTERROR(format, ...) LOG_CHANNEL_PROTO(BSP_LOG_CHANNEL_BOOT, "E:", RTT_CTRL_TEXT_BRIGHT_RED, format, ##__VA_ARGS__)
#define LOGDIAGINFO(format, ...) LOG_CHANNEL_PROTO(BSP_LOG_CHANNEL_DIAG, "I:", RTT_CTRL_TEXT_BRIGHT_GREEN, format, ##__VA_ARGS__)
#define LOGDIAGWARNING(format, ...) LOG_CHANNEL_PROTO(BSP_LOG_CHANNEL_DIAG, "W:", RTT_CTRL_TEXT_BRIGHT_YELLOW, format, ##__VA_ARGS__)
#define LOGDIAGERROR(format, ...) LOG_CHANNEL_PROTO(BSP_LOG_CHANNEL_DIAG, "E:", RTT_CTRL_TEXT_BRIGHT_RED, format, ##__VA_ARGS__)
#endif

int PrintLog(const char *fmt, ...);
int PrintLogChannel(BSPLogChannel channel, const char *fmt, ...);
void Float2Str(char *str, float va);

#endif
