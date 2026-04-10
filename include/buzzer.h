#ifndef BUZZER_H
#define BUZZER_H

typedef enum
{
    OCTAVE_1 = 0,
    OCTAVE_2,
    OCTAVE_3,
    OCTAVE_4,
    OCTAVE_5,
    OCTAVE_6,
    OCTAVE_7,
    OCTAVE_8,
} octave_e;

typedef enum
{
    ALARM_LEVEL_HIGH = 0,
    ALARM_LEVEL_ABOVE_MEDIUM,
    ALARM_LEVEL_MEDIUM,
    ALARM_LEVEL_BELOW_MEDIUM,
    ALARM_LEVEL_LOW,
} AlarmLevel_e;

typedef enum
{
    ALARM_OFF = 0,
    ALARM_ON,
} AlarmState_e;

typedef struct
{
    AlarmLevel_e alarm_level;
    octave_e octave;
    float loudness;
} Buzzer_config_s;

typedef struct
{
    float loudness;
    octave_e octave;
    AlarmLevel_e alarm_level;
    AlarmState_e alarm_state;
} BuzzzerInstance;

void BuzzerInit(void);
void BuzzerTask(void);
BuzzzerInstance *BuzzerRegister(Buzzer_config_s *config);
void AlarmSetStatus(BuzzzerInstance *buzzer, AlarmState_e state);

#endif /* BUZZER_H */
