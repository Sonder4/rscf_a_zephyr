#ifndef MHALL_H_
#define MHALL_H_

#include <stdint.h>

#define MHALL_DEVICE_CNT 8U
#define MHALL_TASK_PERIOD_MS 10U

typedef enum
{
    MHALL_DIRECTION_STOP = 0,
    MHALL_DIRECTION_FORWARD = 1,
    MHALL_DIRECTION_REVERSE = -1,
} MHallDirection_e;

typedef struct
{
    int32_t count;
    int32_t delta_count;
    int8_t direction;
    uint32_t update_tick;
    uint32_t sample_seq;
} MHall_Data_s;

typedef struct
{
    const char *topic_name;
    uint8_t reverse;
    uint32_t counter_period;
} MHall_Init_Config_s;

typedef struct mhall_ins
{
    MHall_Data_s data;
    int32_t last_counter;
    uint32_t counter_period;
    uint8_t reverse;
    uint8_t active;
} MHallInstance;

MHallInstance *MHallRegister(MHall_Init_Config_s *config);
void MHallFeedCount(MHallInstance *instance, int32_t current_counter);
void MHallReset(MHallInstance *instance);
void MHallUpdate(MHallInstance *instance);
void MHallTask(void);
int32_t MHallGetCount(MHallInstance *instance);
MHallDirection_e MHallGetDirection(MHallInstance *instance);

#endif /* MHALL_H_ */
