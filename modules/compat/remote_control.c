#include <stdlib.h>
#include <string.h>

#include "bsp_usart.h"
#include "daemon.h"
#include "remote_control.h"

#define REMOTE_CONTROL_FRAME_SIZE 18U

static RC_ctrl_t s_rc_ctrl[2];
static uint8_t s_rc_init_flag;
static USARTInstance *s_rc_usart_instance;
static DaemonInstance *s_rc_daemon_instance;

static void RectifyRCjoystick(void)
{
    for (uint8_t index = 0U; index < 5U; ++index)
    {
        if (abs(*(&s_rc_ctrl[TEMP].rc.rocker_l_ + index)) > 660)
        {
            *(&s_rc_ctrl[TEMP].rc.rocker_l_ + index) = 0;
        }
    }
}

static void sbus_to_rc(const uint8_t *sbus_buf)
{
    uint16_t key_now;
    uint16_t key_last;
    uint16_t key_with_ctrl;
    uint16_t key_with_shift;
    uint16_t key_last_with_ctrl;
    uint16_t key_last_with_shift;

    s_rc_ctrl[TEMP].rc.rocker_r_ =
        ((sbus_buf[0] | (sbus_buf[1] << 8)) & 0x07FF) - RC_CH_VALUE_OFFSET;
    s_rc_ctrl[TEMP].rc.rocker_r1 =
        (((sbus_buf[1] >> 3) | (sbus_buf[2] << 5)) & 0x07FF) - RC_CH_VALUE_OFFSET;
    s_rc_ctrl[TEMP].rc.rocker_l_ =
        (((sbus_buf[2] >> 6) | (sbus_buf[3] << 2) | (sbus_buf[4] << 10)) & 0x07FF) - RC_CH_VALUE_OFFSET;
    s_rc_ctrl[TEMP].rc.rocker_l1 =
        (((sbus_buf[4] >> 1) | (sbus_buf[5] << 7)) & 0x07FF) - RC_CH_VALUE_OFFSET;
    s_rc_ctrl[TEMP].rc.dial =
        ((sbus_buf[16] | (sbus_buf[17] << 8)) & 0x07FF) - RC_CH_VALUE_OFFSET;
    RectifyRCjoystick();
    s_rc_ctrl[TEMP].rc.switch_right = (uint8_t)((sbus_buf[5] >> 4) & 0x03U);
    s_rc_ctrl[TEMP].rc.switch_left = (uint8_t)(((sbus_buf[5] >> 4) & 0x0CU) >> 2);
    s_rc_ctrl[TEMP].mouse.x = (int16_t)(sbus_buf[6] | (sbus_buf[7] << 8));
    s_rc_ctrl[TEMP].mouse.y = (int16_t)(sbus_buf[8] | (sbus_buf[9] << 8));
    s_rc_ctrl[TEMP].mouse.press_l = sbus_buf[12];
    s_rc_ctrl[TEMP].mouse.press_r = sbus_buf[13];
    *(uint16_t *)&s_rc_ctrl[TEMP].key[KEY_PRESS] = (uint16_t)(sbus_buf[14] | (sbus_buf[15] << 8));

    if (s_rc_ctrl[TEMP].key[KEY_PRESS].ctrl)
    {
        s_rc_ctrl[TEMP].key[KEY_PRESS_WITH_CTRL] = s_rc_ctrl[TEMP].key[KEY_PRESS];
    }
    else
    {
        memset(&s_rc_ctrl[TEMP].key[KEY_PRESS_WITH_CTRL], 0, sizeof(Key_t));
    }

    if (s_rc_ctrl[TEMP].key[KEY_PRESS].shift)
    {
        s_rc_ctrl[TEMP].key[KEY_PRESS_WITH_SHIFT] = s_rc_ctrl[TEMP].key[KEY_PRESS];
    }
    else
    {
        memset(&s_rc_ctrl[TEMP].key[KEY_PRESS_WITH_SHIFT], 0, sizeof(Key_t));
    }

    key_now = s_rc_ctrl[TEMP].key[KEY_PRESS].keys;
    key_last = s_rc_ctrl[LAST].key[KEY_PRESS].keys;
    key_with_ctrl = s_rc_ctrl[TEMP].key[KEY_PRESS_WITH_CTRL].keys;
    key_with_shift = s_rc_ctrl[TEMP].key[KEY_PRESS_WITH_SHIFT].keys;
    key_last_with_ctrl = s_rc_ctrl[LAST].key[KEY_PRESS_WITH_CTRL].keys;
    key_last_with_shift = s_rc_ctrl[LAST].key[KEY_PRESS_WITH_SHIFT].keys;

    for (uint16_t index = 0U, mask = 0x1U; index < 16U; mask <<= 1U, ++index)
    {
        if ((index == 4U) || (index == 5U))
        {
            continue;
        }

        if ((key_now & mask) && !(key_last & mask) && !(key_with_ctrl & mask) && !(key_with_shift & mask))
        {
            s_rc_ctrl[TEMP].key_count[KEY_PRESS][index]++;
        }
        if ((key_with_ctrl & mask) && !(key_last_with_ctrl & mask))
        {
            s_rc_ctrl[TEMP].key_count[KEY_PRESS_WITH_CTRL][index]++;
        }
        if ((key_with_shift & mask) && !(key_last_with_shift & mask))
        {
            s_rc_ctrl[TEMP].key_count[KEY_PRESS_WITH_SHIFT][index]++;
        }
    }

    memcpy(&s_rc_ctrl[LAST], &s_rc_ctrl[TEMP], sizeof(RC_ctrl_t));
}

static void RemoteControlRxCallback(void *instance, uint8_t *data, uint16_t len)
{
    UNUSED(instance);

    if ((data == NULL) || (len < REMOTE_CONTROL_FRAME_SIZE))
    {
        return;
    }

    if (s_rc_daemon_instance != NULL)
    {
        DaemonReload(s_rc_daemon_instance);
    }

    sbus_to_rc(data);
}

static void RCLostCallback(void *id, DaemonEvent_e event)
{
    UNUSED(id);
    if (event != DAEMON_EVENT_OFFLINE_ENTER)
    {
        return;
    }

    memset(s_rc_ctrl, 0, sizeof(s_rc_ctrl));
    if (s_rc_usart_instance != NULL)
    {
        USARTServiceInit(s_rc_usart_instance);
    }
}

RC_ctrl_t *RemoteControlInit(void *rc_usart_handle)
{
    USART_Init_Config_s conf = {0};
    Daemon_Init_Config_s daemon_conf = {0};

    if (s_rc_init_flag != 0U)
    {
        return s_rc_ctrl;
    }

    memset(s_rc_ctrl, 0, sizeof(s_rc_ctrl));
    conf.module_callback = RemoteControlRxCallback;
    conf.moduleinstance = NULL;
    conf.usart_handle = (rc_usart_handle != NULL) ? (UART_HandleTypeDef *)rc_usart_handle : &huart1;
    conf.recv_buff_size = REMOTE_CONTROL_FRAME_SIZE;
    s_rc_usart_instance = USARTRegister(&conf);

    daemon_conf.reload_count = 10U;
    daemon_conf.init_count = 10U;
    daemon_conf.remind_count = 0U;
    daemon_conf.callback = RCLostCallback;
    daemon_conf.owner_id = NULL;
    s_rc_daemon_instance = DaemonRegister(&daemon_conf);
    s_rc_init_flag = 1U;
    return s_rc_ctrl;
}

uint8_t RemoteControlIsOnline(void)
{
    if ((s_rc_init_flag == 0U) || (s_rc_daemon_instance == NULL))
    {
        return 0U;
    }

    return DaemonIsOnline(s_rc_daemon_instance);
}

uint8_t RemoteControlTryGetLatest(RC_ctrl_t *out)
{
    if (out == NULL)
    {
        return 0U;
    }

    *out = s_rc_ctrl[TEMP];
    return 1U;
}
