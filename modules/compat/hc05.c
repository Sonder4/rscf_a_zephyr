#include <string.h>

#include <zephyr/kernel.h>

#include "HC05.h"
#include "daemon.h"
#include "main.h"

#define HC05_BUFFERSIZE (HC05_DATASIZE + 2U)
#define FRAME_HEAD 0xAAU
#define FRAME_END 0x55U

static HC05 s_hc05_msg;
static USARTInstance *s_hc05_usart_instance;
static DaemonInstance *s_hc05_daemon_instance;

static void HC05OfflineCallback(void *id, DaemonEvent_e event)
{
    UNUSED(id);
    if ((event == DAEMON_EVENT_OFFLINE_ENTER) && (s_hc05_usart_instance != NULL))
    {
        s_hc05_msg.online = 0U;
        USARTServiceInit(s_hc05_usart_instance);
    }
}

static void HC05RxCallback(void *instance, uint8_t *data, uint16_t len)
{
    UNUSED(instance);

    if ((data == NULL) || (len < HC05_BUFFERSIZE))
    {
        return;
    }

    if ((data[0] != FRAME_HEAD) || (data[HC05_BUFFERSIZE - 1U] != FRAME_END))
    {
        return;
    }

    memcpy(s_hc05_msg.recv_data, &data[1], HC05_DATASIZE);
    s_hc05_msg.update_tick = k_uptime_get_32();
    s_hc05_msg.sample_seq++;
    s_hc05_msg.online = 1U;
    if (s_hc05_daemon_instance != NULL)
    {
        DaemonReload(s_hc05_daemon_instance);
    }
}

HC05 *HC05Init(UART_HandleTypeDef *hc05_usart_handle)
{
    USART_Init_Config_s conf = {0};
    Daemon_Init_Config_s daemon_conf = {0};

    if (s_hc05_usart_instance != NULL)
    {
        return &s_hc05_msg;
    }

    memset(&s_hc05_msg, 0, sizeof(s_hc05_msg));
    conf.module_callback = HC05RxCallback;
    conf.usart_handle = (hc05_usart_handle != NULL) ? hc05_usart_handle : &huart2;
    conf.recv_buff_size = HC05_BUFFERSIZE;
    conf.moduleinstance = NULL;
    s_hc05_usart_instance = USARTRegister(&conf);

    daemon_conf.reload_count = 30U;
    daemon_conf.init_count = 30U;
    daemon_conf.remind_count = 0U;
    daemon_conf.callback = HC05OfflineCallback;
    daemon_conf.owner_id = NULL;
    s_hc05_daemon_instance = DaemonRegister(&daemon_conf);
    return &s_hc05_msg;
}

void HC05_SendData(uint8_t *data, uint8_t data_num)
{
    if ((s_hc05_usart_instance == NULL) || (data == NULL))
    {
        return;
    }

    if (data_num > HC05_DATASIZE)
    {
        data_num = HC05_DATASIZE;
    }

    memset(s_hc05_msg.send_data, 0, sizeof(s_hc05_msg.send_data));
    s_hc05_msg.send_data[0] = FRAME_HEAD;
    memcpy(&s_hc05_msg.send_data[1], data, data_num);
    s_hc05_msg.send_data[HC05_BUFFERSIZE - 1U] = FRAME_END;
    USARTSend(s_hc05_usart_instance, s_hc05_msg.send_data, data_num + 2U, USART_TRANSFER_IT);
}

uint8_t HC05TryGetLatest(HC05 *out)
{
    if (out == NULL)
    {
        return 0U;
    }

    *out = s_hc05_msg;
    return (s_hc05_msg.sample_seq != 0U) ? 1U : 0U;
}

uint8_t HC05IsOnline(void)
{
    if (s_hc05_daemon_instance == NULL)
    {
        return s_hc05_msg.online;
    }

    return DaemonIsOnline(s_hc05_daemon_instance);
}
