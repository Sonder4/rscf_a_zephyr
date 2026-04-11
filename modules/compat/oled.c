#include <string.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>

#include "oled.h"

static char s_oled_lines[OLED_LINE_COUNT][OLED_LINE_LEN + 1U];
static const struct device *s_i2c2_dev = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(i2c2));
static uint8_t s_oled_ready;

int OLEDInit(void)
{
    OLEDClear();
    s_oled_ready = ((s_i2c2_dev != NULL) && device_is_ready(s_i2c2_dev)) ? 1U : 0U;
    return 0;
}

void OLEDClear(void)
{
    memset(s_oled_lines, ' ', sizeof(s_oled_lines));
    for (uint8_t row = 0U; row < OLED_LINE_COUNT; ++row)
    {
        s_oled_lines[row][OLED_LINE_LEN] = '\0';
    }
}

void OLEDShowString(uint8_t row, uint8_t col, const char *str)
{
    uint8_t write_col = col;

    if ((row >= OLED_LINE_COUNT) || (col >= OLED_LINE_LEN) || (str == NULL))
    {
        return;
    }

    while ((*str != '\0') && (write_col < OLED_LINE_LEN))
    {
        s_oled_lines[row][write_col++] = *str++;
    }
}

const char *OLEDGetLine(uint8_t row)
{
    if (row >= OLED_LINE_COUNT)
    {
        return "";
    }

    return s_oled_lines[row];
}

void OLEDTask(void)
{
    if (s_oled_ready == 0U)
    {
        return;
    }
}
