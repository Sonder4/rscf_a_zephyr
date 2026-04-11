#include <string.h>

#include "master_process.h"
#include "daemon.h"
#include "main.h"

static USARTInstance *s_vision_usart_instance;
static Vision_Recv_s s_recv_data;
static Vision_Send_s s_send_data;
static DaemonInstance *s_vision_daemon_instance;

static void VisionOfflineCallback(void *id, DaemonEvent_e event)
{
    UNUSED(id);
    if ((event == DAEMON_EVENT_OFFLINE_ENTER) && (s_vision_usart_instance != NULL))
    {
        USARTServiceInit(s_vision_usart_instance);
    }
}

static void VisionDecode(void *instance, uint8_t *data, uint16_t len)
{
    uint16_t flag_register = 0U;

    UNUSED(instance);
    UNUSED(len);
    if (data == NULL)
    {
        return;
    }

    (void)get_protocol_info(data, &flag_register, (uint8_t *)&s_recv_data.pitch);
    if (s_vision_daemon_instance != NULL)
    {
        DaemonReload(s_vision_daemon_instance);
    }
}

Vision_Recv_s *VisionInit(UART_HandleTypeDef *handle)
{
    USART_Init_Config_s conf = {0};
    Daemon_Init_Config_s daemon_conf = {0};

    if (s_vision_usart_instance != NULL)
    {
        return &s_recv_data;
    }

    memset(&s_recv_data, 0, sizeof(s_recv_data));
    memset(&s_send_data, 0, sizeof(s_send_data));
    conf.module_callback = VisionDecode;
    conf.recv_buff_size = VISION_RECV_SIZE;
    conf.usart_handle = (handle != NULL) ? handle : &huart3;
    conf.moduleinstance = NULL;
    s_vision_usart_instance = USARTRegister(&conf);

    daemon_conf.reload_count = 20U;
    daemon_conf.init_count = 20U;
    daemon_conf.remind_count = 0U;
    daemon_conf.callback = VisionOfflineCallback;
    daemon_conf.owner_id = NULL;
    s_vision_daemon_instance = DaemonRegister(&daemon_conf);
    return &s_recv_data;
}

void VisionSetFlag(Enemy_Color_e enemy_color, Work_Mode_e work_mode, Bullet_Speed_e bullet_speed)
{
    s_send_data.enemy_color = enemy_color;
    s_send_data.work_mode = work_mode;
    s_send_data.bullet_speed = bullet_speed;
}

void VisionSetAltitude(float yaw, float pitch, float roll)
{
    s_send_data.yaw = yaw;
    s_send_data.pitch = pitch;
    s_send_data.roll = roll;
}

void VisionSend(void)
{
    static uint8_t send_buff[VISION_SEND_SIZE];
    static uint16_t tx_len;
    static uint16_t flag_register;

    if (s_vision_usart_instance == NULL)
    {
        return;
    }

    flag_register = (uint16_t)((uint16_t)30U << 8) | 0x01U;
    get_protocol_send_data(0x02U, flag_register, &s_send_data.yaw, 3U, send_buff, &tx_len);
    USARTSend(s_vision_usart_instance, send_buff, tx_len, USART_TRANSFER_IT);
}

uint8_t VisionTryGetLatest(Vision_Recv_s *out)
{
    if (out == NULL)
    {
        return 0U;
    }

    *out = s_recv_data;
    return 1U;
}

uint8_t VisionIsOnline(void)
{
    if (s_vision_daemon_instance == NULL)
    {
        return 0U;
    }

    return DaemonIsOnline(s_vision_daemon_instance);
}
