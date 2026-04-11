#ifndef MASTER_PROCESS_H_
#define MASTER_PROCESS_H_

#include <stdint.h>

#include "bsp_usart.h"
#include "seasky_protocol.h"

#define VISION_RECV_SIZE 18U
#define VISION_SEND_SIZE 36U

typedef enum
{
    NO_FIRE = 0,
    AUTO_FIRE = 1,
    AUTO_AIM = 2
} Fire_Mode_e;

typedef enum
{
    NO_TARGET = 0,
    TARGET_CONVERGING = 1,
    READY_TO_FIRE = 2
} Target_State_e;

typedef enum
{
    NO_TARGET_NUM = 0,
    HERO1 = 1,
    ENGINEER2 = 2,
    INFANTRY3 = 3,
    INFANTRY4 = 4,
    INFANTRY5 = 5,
    OUTPOST = 6,
    SENTRY = 7,
    BASE = 8
} Target_Type_e;

typedef struct
{
    Fire_Mode_e fire_mode;
    Target_State_e target_state;
    Target_Type_e target_type;
    float pitch;
    float yaw;
} Vision_Recv_s;

typedef enum
{
    COLOR_NONE = 0,
    COLOR_BLUE = 1,
    COLOR_RED = 2,
} Enemy_Color_e;

typedef enum
{
    VISION_MODE_AIM = 0,
    VISION_MODE_SMALL_BUFF = 1,
    VISION_MODE_BIG_BUFF = 2
} Work_Mode_e;

typedef enum
{
    BULLET_SPEED_NONE = 0,
    BIG_AMU_10 = 10,
    SMALL_AMU_15 = 15,
    BIG_AMU_16 = 16,
    SMALL_AMU_18 = 18,
    SMALL_AMU_30 = 30,
} Bullet_Speed_e;

typedef struct
{
    Enemy_Color_e enemy_color;
    Work_Mode_e work_mode;
    Bullet_Speed_e bullet_speed;
    float yaw;
    float pitch;
    float roll;
} Vision_Send_s;

Vision_Recv_s *VisionInit(UART_HandleTypeDef *handle);
void VisionSend(void);
void VisionSetFlag(Enemy_Color_e enemy_color, Work_Mode_e work_mode, Bullet_Speed_e bullet_speed);
void VisionSetAltitude(float yaw, float pitch, float roll);
uint8_t VisionTryGetLatest(Vision_Recv_s *out);
uint8_t VisionIsOnline(void);

#endif /* MASTER_PROCESS_H_ */
