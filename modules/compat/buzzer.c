#include <stdlib.h>
#include <string.h>

#include "buzzer.h"
#include "rscf_buzzer.h"

#define BUZZER_DEVICE_CNT 5U

#define DoFreq 523U
#define ReFreq 587U
#define MiFreq 659U
#define FaFreq 698U
#define SoFreq 784U
#define LaFreq 880U
#define SiFreq 988U

static BuzzzerInstance *s_buzzer_list[BUZZER_DEVICE_CNT];
static uint8_t s_buzzer_inited;

static uint32_t BuzzerOctaveToFreq(octave_e octave)
{
    switch (octave)
    {
    case OCTAVE_1:
        return DoFreq;
    case OCTAVE_2:
        return ReFreq;
    case OCTAVE_3:
        return MiFreq;
    case OCTAVE_4:
        return FaFreq;
    case OCTAVE_5:
        return SoFreq;
    case OCTAVE_6:
        return LaFreq;
    case OCTAVE_7:
        return SiFreq;
    case OCTAVE_8:
    default:
        return DoFreq * 2U;
    }
}

void BuzzerInit(void)
{
    memset(s_buzzer_list, 0, sizeof(s_buzzer_list));
    (void)RSCFBuzzerInit();
    s_buzzer_inited = 1U;
}

BuzzzerInstance *BuzzerRegister(Buzzer_config_s *config)
{
    BuzzzerInstance *instance;

    if ((config == NULL) || (config->alarm_level > ALARM_LEVEL_LOW))
    {
        return NULL;
    }

    instance = (BuzzzerInstance *)malloc(sizeof(BuzzzerInstance));
    if (instance == NULL)
    {
        return NULL;
    }

    memset(instance, 0, sizeof(BuzzzerInstance));
    instance->alarm_level = config->alarm_level;
    instance->loudness = config->loudness;
    instance->octave = config->octave;
    instance->alarm_state = ALARM_OFF;
    s_buzzer_list[config->alarm_level] = instance;
    return instance;
}

void AlarmSetStatus(BuzzzerInstance *buzzer, AlarmState_e state)
{
    if (buzzer == NULL)
    {
        return;
    }

    buzzer->alarm_state = state;
}

void BuzzerTask(void)
{
    BuzzzerInstance *active = NULL;
    uint16_t duty_permille;
    float loudness;

    if (s_buzzer_inited == 0U)
    {
        return;
    }

    for (size_t i = 0; i < BUZZER_DEVICE_CNT; ++i)
    {
        if ((s_buzzer_list[i] != NULL) && (s_buzzer_list[i]->alarm_state == ALARM_ON))
        {
            active = s_buzzer_list[i];
            break;
        }
    }

    if (active == NULL)
    {
        RSCFBuzzerStop();
        return;
    }

    loudness = active->loudness;
    if (loudness < 0.0f)
    {
        loudness = 0.0f;
    }
    else if (loudness > 1.0f)
    {
        loudness = 1.0f;
    }

    duty_permille = (uint16_t)(loudness * 1000.0f);
    if (duty_permille == 0U)
    {
        duty_permille = 1U;
    }

    (void)RSCFBuzzerSetTone(BuzzerOctaveToFreq(active->octave), duty_permille);
}
