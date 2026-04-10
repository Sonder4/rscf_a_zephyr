#include "bsp_log.h"

#include "SEGGER_RTT.h"
#include <zephyr/kernel.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BSP_LOG_BOOT_BUFFER_SIZE     2048U
#define BSP_LOG_DIAG_BUFFER_SIZE     12288U
#define BSP_LOG_RUNTIME_BUFFER_SIZE  8192U
#define BSP_LOG_PLOT_BUFFER_SIZE     8192U
#define BSP_LOG_PLOT_MAX_VALUES      4U
#define BSP_LOG_BINARY_MAX_FRAME_SIZE 96U
#define BSP_LOG_PRINTF_BUFFER_SIZE   256U
#define BSP_LOG_PLOT_STREAM_DEFAULT  "plot"
#define BSP_LOG_TOKEN_FALLBACK       "na"
#define BSP_LOG_CHANNEL_FLAGS        SEGGER_RTT_MODE_NO_BLOCK_TRIM

typedef struct
{
    volatile uint32_t write_count;
    volatile uint32_t drop_count;
    volatile uint32_t seq;
} BSPLogChannelStats;

char g_bsp_log_rtt_boot_up_buffer[BSP_LOG_BOOT_BUFFER_SIZE];
char g_bsp_log_rtt_diag_up_buffer[BSP_LOG_DIAG_BUFFER_SIZE];
char g_bsp_log_rtt_runtime_up_buffer[BSP_LOG_RUNTIME_BUFFER_SIZE];
char g_bsp_log_rtt_plot_up_buffer[BSP_LOG_PLOT_BUFFER_SIZE];
volatile uint8_t g_bsp_log_plot_enabled = 0U;
__attribute__((used)) static volatile uintptr_t g_bsp_log_plot_exports[4];
static BSPLogChannelStats g_bsp_log_channel_stats[BSP_LOG_CHANNEL_COUNT];
static const char *g_bsp_log_drop_counter_names[BSP_LOG_CHANNEL_COUNT] = {"boot_drop_count", "diag_drop_count", "runtime_drop_count", "plot_drop_count"};
static const char *g_bsp_log_seq_counter_names[BSP_LOG_CHANNEL_COUNT] = {"boot_seq", "diag_seq", "runtime_seq", "plot_seq"};
static const unsigned s_bsp_log_rtt_channel_map[BSP_LOG_CHANNEL_COUNT] = {1U, 2U, 3U, 4U};

static uint32_t BSPLogGetTickMs(void)
{
    return k_uptime_get_32();
}

unsigned int BSPLogChannelToRtt(BSPLogChannel channel)
{
    if (channel >= BSP_LOG_CHANNEL_COUNT)
    {
        return 0U;
    }

    return s_bsp_log_rtt_channel_map[(unsigned)channel];
}

static void BSPLogConfigChannel(BSPLogChannel channel, const char *name, char *buffer, unsigned buffer_size)
{
    unsigned int rtt_channel = BSPLogChannelToRtt(channel);

    SEGGER_RTT_ConfigUpBuffer(rtt_channel, name, buffer, buffer_size, BSP_LOG_CHANNEL_FLAGS);
    SEGGER_RTT_SetFlagsUpBuffer(rtt_channel, BSP_LOG_CHANNEL_FLAGS);
}

static void BSPLogSanitizeToken(char *dst, size_t dst_size, const char *src)
{
    size_t write_index;

    if ((dst == NULL) || (dst_size == 0U))
    {
        return;
    }

    if (src == NULL)
    {
        src = BSP_LOG_TOKEN_FALLBACK;
    }

    write_index = 0U;
    while ((*src != '\0') && (write_index + 1U < dst_size))
    {
        char ch = *src++;
        if ((ch == ',') || (ch == '\r') || (ch == '\n'))
        {
            ch = '_';
        }
        dst[write_index++] = ch;
    }

    if (write_index == 0U)
    {
        const char *fallback = BSP_LOG_TOKEN_FALLBACK;
        while ((*fallback != '\0') && (write_index + 1U < dst_size))
        {
            dst[write_index++] = *fallback++;
        }
    }

    dst[write_index] = '\0';
}

static void BSPLogWriteLine(BSPLogChannel channel, const char *line)
{
    unsigned written;
    size_t expected;

    if ((channel >= BSP_LOG_CHANNEL_COUNT) || (line == NULL))
    {
        return;
    }

    expected = strlen(line);
    written = SEGGER_RTT_Write(BSPLogChannelToRtt(channel), line, expected);
    g_bsp_log_channel_stats[(unsigned)channel].write_count++;
    if ((size_t)written != expected)
    {
        g_bsp_log_channel_stats[(unsigned)channel].drop_count++;
    }
}

static void BSPLogWriteFrame(BSPLogChannel channel, const uint8_t *frame, uint16_t frame_len)
{
    unsigned written;

    if ((channel >= BSP_LOG_CHANNEL_COUNT) || (frame == NULL) || (frame_len == 0U))
    {
        return;
    }

    written = SEGGER_RTT_Write(BSPLogChannelToRtt(channel), frame, frame_len);
    g_bsp_log_channel_stats[(unsigned)channel].write_count++;
    if (written != (unsigned)frame_len)
    {
        g_bsp_log_channel_stats[(unsigned)channel].drop_count++;
    }
}

static int BSPLogVPrintf(BSPLogChannel channel, const char *fmt, va_list args)
{
    char buffer[BSP_LOG_PRINTF_BUFFER_SIZE];
    int written;

    if ((channel >= BSP_LOG_CHANNEL_COUNT) || (fmt == NULL))
    {
        return -1;
    }

    written = vsnprintf(buffer, sizeof(buffer), fmt, args);
    if (written < 0)
    {
        return written;
    }

    if ((size_t)written >= sizeof(buffer))
    {
        g_bsp_log_channel_stats[(unsigned)channel].drop_count++;
        buffer[sizeof(buffer) - 1U] = '\0';
    }

    BSPLogWriteLine(channel, buffer);
    return written;
}

static void BSPLogBinaryFillHeader(
    BSPLogBinaryFrameHeader *header,
    BSPLogBinaryFrameKind kind,
    uint16_t payload_len,
    uint16_t flags,
    uint16_t seq,
    uint16_t tick_ms)
{
    if (header == NULL)
    {
        return;
    }

    header->magic = (uint16_t)BSP_LOG_BINARY_MAGIC;
    header->version = BSP_LOG_BINARY_VERSION;
    header->kind = (uint8_t)kind;
    header->payload_len = payload_len;
    header->flags = flags;
    header->seq = seq;
    header->tick_ms = tick_ms;
}

static uint32_t BSPLogNextSeq(BSPLogChannel channel)
{
    if (channel >= BSP_LOG_CHANNEL_COUNT)
    {
        return 0U;
    }
    g_bsp_log_channel_stats[(unsigned)channel].seq++;
    return g_bsp_log_channel_stats[(unsigned)channel].seq;
}

static void BSPLogSetSeq(BSPLogChannel channel, uint32_t seq)
{
    if (channel >= BSP_LOG_CHANNEL_COUNT)
    {
        return;
    }
    g_bsp_log_channel_stats[(unsigned)channel].seq = seq;
}

void BSPLogInit(void)
{
    SEGGER_RTT_Init();

    BSPLogConfigChannel(BSP_LOG_CHANNEL_BOOT, "boot", g_bsp_log_rtt_boot_up_buffer, BSP_LOG_BOOT_BUFFER_SIZE);
    BSPLogConfigChannel(BSP_LOG_CHANNEL_DIAG, "diag", g_bsp_log_rtt_diag_up_buffer, BSP_LOG_DIAG_BUFFER_SIZE);
    BSPLogConfigChannel(BSP_LOG_CHANNEL_RUNTIME, "runtime", g_bsp_log_rtt_runtime_up_buffer, BSP_LOG_RUNTIME_BUFFER_SIZE);
    BSPLogConfigChannel(BSP_LOG_CHANNEL_PLOT, "plot", g_bsp_log_rtt_plot_up_buffer, BSP_LOG_PLOT_BUFFER_SIZE);

    g_bsp_log_plot_exports[0] = (uintptr_t)&g_bsp_log_plot_enabled;
    g_bsp_log_plot_exports[1] = (uintptr_t)&BSPLogPlotSetEnabled;
    g_bsp_log_plot_exports[2] = (uintptr_t)&BSPLogPlotIsEnabled;
    g_bsp_log_plot_exports[3] = (uintptr_t)&BSPLogPlotF32;

    PrintLogChannel(BSP_LOG_CHANNEL_BOOT, "Log Init!\n");
    BSPLogDiagMeta("diag_protocol", "v1");
    BSPLogDiagCounter(g_bsp_log_drop_counter_names[BSP_LOG_CHANNEL_PLOT], 0U);
    BSPLogDiagCounter(g_bsp_log_seq_counter_names[BSP_LOG_CHANNEL_PLOT], 0U);
}

void BSPLogPlotSetEnabled(uint8_t enabled)
{
    g_bsp_log_plot_enabled = enabled ? 1U : 0U;
}

uint8_t BSPLogPlotIsEnabled(void)
{
    return g_bsp_log_plot_enabled;
}

void BSPLogBinaryEmitDict(BSPLogChannel channel, uint8_t dict_kind, uint8_t entry_id, const char *name, uint16_t seq, uint16_t tick_ms)
{
    uint8_t frame[BSP_LOG_BINARY_MAX_FRAME_SIZE];
    BSPLogBinaryFrameHeader header;
    char name_token[32];
    uint16_t name_len;
    uint16_t frame_len;

    BSPLogSanitizeToken(name_token, sizeof(name_token), name);
    name_len = (uint16_t)strlen(name_token);
    frame_len = (uint16_t)(sizeof(BSPLogBinaryFrameHeader) + 4U + name_len);
    if (frame_len > sizeof(frame))
    {
        return;
    }

    BSPLogBinaryFillHeader(
        &header,
        BSP_LOG_BINARY_FRAME_KIND_META_DICT,
        (uint16_t)(4U + name_len),
        0U,
        seq,
        tick_ms);
    memcpy(frame, &header, sizeof(header));
    frame[sizeof(header) + 0U] = dict_kind;
    frame[sizeof(header) + 1U] = entry_id;
    frame[sizeof(header) + 2U] = (uint8_t)(name_len & 0xFFU);
    frame[sizeof(header) + 3U] = (uint8_t)((name_len >> 8) & 0xFFU);
    memcpy(&frame[sizeof(header) + 4U], name_token, name_len);
    BSPLogWriteFrame(channel, frame, frame_len);
}

void BSPLogBinaryEmitSchema(
    BSPLogChannel channel,
    uint8_t schema_id,
    uint8_t schema_kind,
    uint8_t value_type,
    int8_t scale_pow10,
    const uint8_t *field_ids,
    uint8_t field_count,
    uint16_t seq,
    uint16_t tick_ms)
{
    uint8_t frame[BSP_LOG_BINARY_MAX_FRAME_SIZE];
    BSPLogBinaryFrameHeader header;
    uint16_t payload_len;
    uint16_t frame_len;

    payload_len = (uint16_t)(6U + field_count);
    frame_len = (uint16_t)(sizeof(BSPLogBinaryFrameHeader) + payload_len);
    if (frame_len > sizeof(frame))
    {
        return;
    }

    BSPLogBinaryFillHeader(
        &header,
        BSP_LOG_BINARY_FRAME_KIND_META_SCHEMA,
        payload_len,
        0U,
        seq,
        tick_ms);
    memcpy(frame, &header, sizeof(header));
    frame[sizeof(header) + 0U] = schema_id;
    frame[sizeof(header) + 1U] = schema_kind;
    frame[sizeof(header) + 2U] = value_type;
    frame[sizeof(header) + 3U] = field_count;
    frame[sizeof(header) + 4U] = (uint8_t)scale_pow10;
    frame[sizeof(header) + 5U] = 0U;
    if ((field_count > 0U) && (field_ids != NULL))
    {
        memcpy(&frame[sizeof(header) + 6U], field_ids, field_count);
    }
    BSPLogWriteFrame(channel, frame, frame_len);
}

void BSPLogBinaryEmitDiagData(
    BSPLogChannel channel,
    uint8_t module_id,
    uint8_t event_id,
    uint8_t schema_id,
    const int32_t *values,
    uint8_t value_count,
    uint16_t seq,
    uint16_t tick_ms,
    uint16_t flags)
{
    uint8_t frame[BSP_LOG_BINARY_MAX_FRAME_SIZE];
    BSPLogBinaryFrameHeader header;
    uint16_t payload_len;
    uint16_t frame_len;
    uint32_t index;
    uint16_t offset;

    payload_len = (uint16_t)(4U + ((uint16_t)value_count * 4U));
    frame_len = (uint16_t)(sizeof(BSPLogBinaryFrameHeader) + payload_len);
    if (frame_len > sizeof(frame))
    {
        return;
    }

    BSPLogBinaryFillHeader(
        &header,
        BSP_LOG_BINARY_FRAME_KIND_DIAG_DATA,
        payload_len,
        flags,
        seq,
        tick_ms);
    memcpy(frame, &header, sizeof(header));
    frame[sizeof(header) + 0U] = module_id;
    frame[sizeof(header) + 1U] = event_id;
    frame[sizeof(header) + 2U] = schema_id;
    frame[sizeof(header) + 3U] = value_count;

    offset = (uint16_t)(sizeof(header) + 4U);
    for (index = 0U; index < value_count; ++index)
    {
        int32_t value = (values != NULL) ? values[index] : 0;
        memcpy(&frame[offset], &value, sizeof(value));
        offset = (uint16_t)(offset + sizeof(value));
    }

    BSPLogWriteFrame(channel, frame, frame_len);
}

void BSPLogBinaryEmitPlotData(
    BSPLogChannel channel,
    uint8_t stream_id,
    uint8_t value_type,
    int8_t scale_pow10,
    const int32_t *values,
    uint8_t value_count,
    uint16_t seq,
    uint16_t tick_ms,
    uint16_t flags)
{
    uint8_t frame[BSP_LOG_BINARY_MAX_FRAME_SIZE];
    BSPLogBinaryFrameHeader header;
    uint16_t payload_len;
    uint16_t frame_len;
    uint32_t index;
    uint16_t offset;

    payload_len = (uint16_t)(4U + ((uint16_t)value_count * 4U));
    frame_len = (uint16_t)(sizeof(BSPLogBinaryFrameHeader) + payload_len);
    if (frame_len > sizeof(frame))
    {
        return;
    }

    BSPLogBinaryFillHeader(
        &header,
        BSP_LOG_BINARY_FRAME_KIND_PLOT_DATA,
        payload_len,
        flags,
        seq,
        tick_ms);
    memcpy(frame, &header, sizeof(header));
    frame[sizeof(header) + 0U] = stream_id;
    frame[sizeof(header) + 1U] = value_type;
    frame[sizeof(header) + 2U] = value_count;
    frame[sizeof(header) + 3U] = (uint8_t)scale_pow10;

    offset = (uint16_t)(sizeof(header) + 4U);
    for (index = 0U; index < value_count; ++index)
    {
        int32_t value = (values != NULL) ? values[index] : 0;
        memcpy(&frame[offset], &value, sizeof(value));
        offset = (uint16_t)(offset + sizeof(value));
    }

    BSPLogWriteFrame(channel, frame, frame_len);
}

void BSPLogPlotF32(const char *stream, const float *values, uint32_t count)
{
    char line[192];
    char value_str[24];
    const char *stream_name;
    char stream_token[32];
    int step;
    uint32_t index;
    uint32_t seq;
    size_t used;

    if ((values == NULL) || (count == 0U) || (count > BSP_LOG_PLOT_MAX_VALUES) || (BSPLogPlotIsEnabled() == 0U))
    {
        return;
    }

    stream_name = (stream != NULL) ? stream : BSP_LOG_PLOT_STREAM_DEFAULT;
    BSPLogSanitizeToken(stream_token, sizeof(stream_token), stream_name);
    seq = BSPLogNextSeq(BSP_LOG_CHANNEL_PLOT);
    step = snprintf(
        line,
        sizeof(line),
        "@plot2,%s,%lu,%lu,%lu",
        stream_token,
        (unsigned long)BSPLogGetTickMs(),
        (unsigned long)seq,
        (unsigned long)g_bsp_log_channel_stats[BSP_LOG_CHANNEL_PLOT].drop_count);
    if ((step <= 0) || ((size_t)step >= sizeof(line)))
    {
        return;
    }
    used = (size_t)step;

    for (index = 0U; index < count; ++index)
    {
        Float2Str(value_str, values[index]);
        step = snprintf(&line[used], sizeof(line) - used, ",%s", value_str);
        if ((step <= 0) || ((size_t)step >= (sizeof(line) - used)))
        {
            return;
        }
        used += (size_t)step;
    }

    step = snprintf(&line[used], sizeof(line) - used, "\r\n");
    if ((step <= 0) || ((size_t)step >= (sizeof(line) - used)))
    {
        return;
    }

    BSPLogWriteLine(BSP_LOG_CHANNEL_PLOT, line);
}

void BSPLogPlotBenchFrame(const char *stream, uint32_t tick_ms, uint32_t seq, uint32_t channel_mask, uint32_t payload_width)
{
    char line[128];
    char stream_token[32];
    const char *stream_name;
    uint32_t count;
    int step;

    if (BSPLogPlotIsEnabled() == 0U)
    {
        return;
    }

    count = payload_width;
    if (count == 0U)
    {
        count = 1U;
    }
    if (count > BSP_LOG_PLOT_MAX_VALUES)
    {
        count = BSP_LOG_PLOT_MAX_VALUES;
    }

    stream_name = (stream != NULL) ? stream : BSP_LOG_PLOT_STREAM_DEFAULT;
    BSPLogSanitizeToken(stream_token, sizeof(stream_token), stream_name);
    BSPLogSetSeq(BSP_LOG_CHANNEL_PLOT, seq);

    switch (count)
    {
    case 1U:
        step = snprintf(
            line,
            sizeof(line),
            "@p2,%s,%lu,%lu,%lu,%lu\r\n",
            stream_token,
            (unsigned long)tick_ms,
            (unsigned long)seq,
            (unsigned long)g_bsp_log_channel_stats[BSP_LOG_CHANNEL_PLOT].drop_count,
            (unsigned long)seq);
        break;
    case 2U:
        step = snprintf(
            line,
            sizeof(line),
            "@p2,%s,%lu,%lu,%lu,%lu,%lu\r\n",
            stream_token,
            (unsigned long)tick_ms,
            (unsigned long)seq,
            (unsigned long)g_bsp_log_channel_stats[BSP_LOG_CHANNEL_PLOT].drop_count,
            (unsigned long)seq,
            (unsigned long)tick_ms);
        break;
    case 3U:
        step = snprintf(
            line,
            sizeof(line),
            "@p2,%s,%lu,%lu,%lu,%lu,%lu,%lu\r\n",
            stream_token,
            (unsigned long)tick_ms,
            (unsigned long)seq,
            (unsigned long)g_bsp_log_channel_stats[BSP_LOG_CHANNEL_PLOT].drop_count,
            (unsigned long)seq,
            (unsigned long)tick_ms,
            (unsigned long)channel_mask);
        break;
    default:
        step = snprintf(
            line,
            sizeof(line),
            "@p2,%s,%lu,%lu,%lu,%lu,%lu,%lu,%lu\r\n",
            stream_token,
            (unsigned long)tick_ms,
            (unsigned long)seq,
            (unsigned long)g_bsp_log_channel_stats[BSP_LOG_CHANNEL_PLOT].drop_count,
            (unsigned long)seq,
            (unsigned long)tick_ms,
            (unsigned long)channel_mask,
            (unsigned long)payload_width);
        break;
    }

    if ((step <= 0) || ((size_t)step >= sizeof(line)))
    {
        return;
    }

    BSPLogWriteLine(BSP_LOG_CHANNEL_PLOT, line);
}

void BSPLogDiagEvent(BSPLogDiagLevel level, const char *module, const char *event, const char *kv_pairs)
{
    char module_token[32];
    char event_token[32];
    char line[192];
    uint32_t seq;
    int step;

    BSPLogSanitizeToken(module_token, sizeof(module_token), module);
    BSPLogSanitizeToken(event_token, sizeof(event_token), event);
    seq = BSPLogNextSeq(BSP_LOG_CHANNEL_DIAG);
    if ((kv_pairs != NULL) && (kv_pairs[0] != '\0'))
    {
        step = snprintf(
            line,
            sizeof(line),
            "@evt,%lu,%lu,%c,%s,%s,%s\r\n",
            (unsigned long)BSPLogGetTickMs(),
            (unsigned long)seq,
            (char)level,
            module_token,
            event_token,
            kv_pairs);
    }
    else
    {
        step = snprintf(
            line,
            sizeof(line),
            "@evt,%lu,%lu,%c,%s,%s\r\n",
            (unsigned long)BSPLogGetTickMs(),
            (unsigned long)seq,
            (char)level,
            module_token,
            event_token);
    }
    if ((step <= 0) || ((size_t)step >= sizeof(line)))
    {
        return;
    }
    BSPLogWriteLine(BSP_LOG_CHANNEL_DIAG, line);
}

void BSPLogDiagBenchFrame(uint32_t tick_ms, uint32_t seq, uint32_t channel_mask, uint32_t payload_width)
{
    char line[64];
    int step;

    BSPLogSetSeq(BSP_LOG_CHANNEL_DIAG, seq);
    step = snprintf(
        line,
        sizeof(line),
        "@d2,%lu,%lu,%lu,%lu\r\n",
        (unsigned long)tick_ms,
        (unsigned long)seq,
        (unsigned long)channel_mask,
        (unsigned long)payload_width);
    if ((step <= 0) || ((size_t)step >= sizeof(line)))
    {
        return;
    }
    BSPLogWriteLine(BSP_LOG_CHANNEL_DIAG, line);
}

void BSPLogDiagMeta(const char *key, const char *value)
{
    char key_token[32];
    char value_token[64];
    char line[128];
    int step;

    BSPLogSanitizeToken(key_token, sizeof(key_token), key);
    BSPLogSanitizeToken(value_token, sizeof(value_token), value);
    step = snprintf(line, sizeof(line), "@meta,%lu,%s,%s\r\n", (unsigned long)BSPLogGetTickMs(), key_token, value_token);
    if ((step <= 0) || ((size_t)step >= sizeof(line)))
    {
        return;
    }
    BSPLogWriteLine(BSP_LOG_CHANNEL_DIAG, line);
}

void BSPLogDiagCounter(const char *name, uint32_t value)
{
    char name_token[32];
    char line[128];
    int step;

    BSPLogSanitizeToken(name_token, sizeof(name_token), name);
    step = snprintf(line, sizeof(line), "@cnt,%lu,%s,%lu\r\n", (unsigned long)BSPLogGetTickMs(), name_token, (unsigned long)value);
    if ((step <= 0) || ((size_t)step >= sizeof(line)))
    {
        return;
    }
    BSPLogWriteLine(BSP_LOG_CHANNEL_DIAG, line);
}

void BSPLogRuntimeBenchFrame(uint32_t tick_ms, uint32_t seq, uint32_t channel_mask, uint32_t payload_width)
{
    char line[64];
    int step;

    BSPLogSetSeq(BSP_LOG_CHANNEL_RUNTIME, seq);
    step = snprintf(
        line,
        sizeof(line),
        "@r2,%lu,%lu,%lu,%lu\r\n",
        (unsigned long)tick_ms,
        (unsigned long)seq,
        (unsigned long)channel_mask,
        (unsigned long)payload_width);
    if ((step <= 0) || ((size_t)step >= sizeof(line)))
    {
        return;
    }
    BSPLogWriteLine(BSP_LOG_CHANNEL_RUNTIME, line);
}

void BSPLogGetChannelStats(BSPLogChannel channel, uint32_t *write_count, uint32_t *drop_count, uint32_t *seq)
{
    if (channel >= BSP_LOG_CHANNEL_COUNT)
    {
        return;
    }
    if (write_count != NULL)
    {
        *write_count = g_bsp_log_channel_stats[(unsigned)channel].write_count;
    }
    if (drop_count != NULL)
    {
        *drop_count = g_bsp_log_channel_stats[(unsigned)channel].drop_count;
    }
    if (seq != NULL)
    {
        *seq = g_bsp_log_channel_stats[(unsigned)channel].seq;
    }
}

void BSPLogResetUpBuffer(void)
{
    BSPLogResetChannel(BSP_LOG_CHANNEL_DIAG);
}

void BSPLogResetChannel(BSPLogChannel channel)
{
    SEGGER_RTT_BUFFER_UP *up_buffer;

    if (channel >= BSP_LOG_CHANNEL_COUNT)
    {
        return;
    }

    up_buffer = &_SEGGER_RTT.aUp[BSPLogChannelToRtt(channel)];
    up_buffer->RdOff = up_buffer->WrOff;
    g_bsp_log_channel_stats[(unsigned)channel].write_count = 0U;
    g_bsp_log_channel_stats[(unsigned)channel].drop_count = 0U;
    g_bsp_log_channel_stats[(unsigned)channel].seq = 0U;
}

int PrintLog(const char *fmt, ...)
{
    va_list args;
    int n;

    va_start(args, fmt);
    n = BSPLogVPrintf(BSP_LOG_CHANNEL_RUNTIME, fmt, args);
    va_end(args);
    return n;
}

int PrintLogChannel(BSPLogChannel channel, const char *fmt, ...)
{
    va_list args;
    int n;

    va_start(args, fmt);
    n = BSPLogVPrintf(channel, fmt, args);
    va_end(args);
    return n;
}

int BSPLogPrintf(BSPLogChannel channel, const char *fmt, ...)
{
    va_list args;
    int n;

    va_start(args, fmt);
    n = BSPLogVPrintf(channel, fmt, args);
    va_end(args);
    return n;
}

void Float2Str(char *str, float va)
{
    int flag = va < 0;
    int head = (int)va;
    int point = (int)((va - head) * 1000);
    head = abs(head);
    point = abs(point);
    if (flag)
        sprintf(str, "-%d.%d", head, point);
    else
        sprintf(str, "%d.%d", head, point);
}
