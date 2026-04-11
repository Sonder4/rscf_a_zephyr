#ifndef REMOTE_CONTROL_H_
#define REMOTE_CONTROL_H_

#include <stdint.h>

#define LAST 1
#define TEMP 0
#define KEY_PRESS 0
#define KEY_STATE 1
#define KEY_PRESS_WITH_CTRL 1
#define KEY_PRESS_WITH_SHIFT 2

#define RC_CH_VALUE_MIN ((uint16_t)364)
#define RC_CH_VALUE_OFFSET ((uint16_t)1024)
#define RC_CH_VALUE_MAX ((uint16_t)1684)

#define RC_SW_UP ((uint16_t)1)
#define RC_SW_MID ((uint16_t)3)
#define RC_SW_DOWN ((uint16_t)2)

typedef union
{
    struct
    {
        uint16_t w : 1;
        uint16_t s : 1;
        uint16_t d : 1;
        uint16_t a : 1;
        uint16_t shift : 1;
        uint16_t ctrl : 1;
        uint16_t q : 1;
        uint16_t e : 1;
        uint16_t r : 1;
        uint16_t f : 1;
        uint16_t g : 1;
        uint16_t z : 1;
        uint16_t x : 1;
        uint16_t c : 1;
        uint16_t v : 1;
        uint16_t b : 1;
    };
    uint16_t keys;
} Key_t;

typedef struct
{
    struct
    {
        int16_t rocker_l_;
        int16_t rocker_l1;
        int16_t rocker_r_;
        int16_t rocker_r1;
        int16_t dial;
        uint8_t switch_left;
        uint8_t switch_right;
    } rc;
    struct
    {
        int16_t x;
        int16_t y;
        uint8_t press_l;
        uint8_t press_r;
    } mouse;
    Key_t key[3];
    uint8_t key_count[3][16];
} RC_ctrl_t;

RC_ctrl_t *RemoteControlInit(void *rc_usart_handle);
uint8_t RemoteControlIsOnline(void);
uint8_t RemoteControlTryGetLatest(RC_ctrl_t *out);

#endif /* REMOTE_CONTROL_H_ */
