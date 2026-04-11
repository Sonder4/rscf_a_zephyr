#include <stdlib.h>
#include <string.h>

#include "user_lib.h"

void MatInit(mat *m, uint8_t row, uint8_t col)
{
    float *data;

    if (m == NULL)
    {
        return;
    }

    data = (float *)zmalloc((size_t)row * (size_t)col * sizeof(float));
    arm_mat_init_f32(m, row, col, data);
}

void *zmalloc(size_t size)
{
    void *ptr = malloc(size);

    if (ptr != NULL)
    {
        memset(ptr, 0, size);
    }

    return ptr;
}

float Sqrt(float x)
{
    float out = 0.0f;

    (void)arm_sqrt_f32(x, &out);
    return out;
}

float abs_limit(float num, float Limit)
{
    if (num > Limit)
    {
        return Limit;
    }
    if (num < -Limit)
    {
        return -Limit;
    }
    return num;
}

float sign(float value)
{
    return (value >= 0.0f) ? 1.0f : -1.0f;
}

float float_deadband(float Value, float minValue, float maxValue)
{
    if ((Value > minValue) && (Value < maxValue))
    {
        return 0.0f;
    }
    return Value;
}

float float_constrain(float Value, float minValue, float maxValue)
{
    if (Value < minValue)
    {
        return minValue;
    }
    if (Value > maxValue)
    {
        return maxValue;
    }
    return Value;
}

int16_t int16_constrain(int16_t Value, int16_t minValue, int16_t maxValue)
{
    if (Value < minValue)
    {
        return minValue;
    }
    if (Value > maxValue)
    {
        return maxValue;
    }
    return Value;
}

float loop_float_constrain(float Input, float minValue, float maxValue)
{
    float range = maxValue - minValue;

    if (range <= 0.0f)
    {
        return minValue;
    }

    while (Input > maxValue)
    {
        Input -= range;
    }
    while (Input < minValue)
    {
        Input += range;
    }

    return Input;
}

float theta_format(float Ang)
{
    return loop_float_constrain(Ang, -180.0f, 180.0f);
}

int float_rounding(float raw)
{
    return (raw >= 0.0f) ? (int)(raw + 0.5f) : (int)(raw - 0.5f);
}

float *Norm3d(float *v)
{
    float norm;

    if (v == NULL)
    {
        return NULL;
    }

    norm = NormOf3d(v);
    if (norm <= 1e-6f)
    {
        return v;
    }

    v[0] /= norm;
    v[1] /= norm;
    v[2] /= norm;
    return v;
}

float NormOf3d(float *v)
{
    if (v == NULL)
    {
        return 0.0f;
    }

    return Sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

void Cross3d(float *v1, float *v2, float *res)
{
    if ((v1 == NULL) || (v2 == NULL) || (res == NULL))
    {
        return;
    }

    res[0] = v1[1] * v2[2] - v1[2] * v2[1];
    res[1] = v1[2] * v2[0] - v1[0] * v2[2];
    res[2] = v1[0] * v2[1] - v1[1] * v2[0];
}

float Dot3d(float *v1, float *v2)
{
    if ((v1 == NULL) || (v2 == NULL))
    {
        return 0.0f;
    }

    return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
}

float AverageFilter(float new_data, float *buf, uint8_t len)
{
    float sum = 0.0f;

    if ((buf == NULL) || (len == 0U))
    {
        return new_data;
    }

    memmove(&buf[1], &buf[0], (size_t)(len - 1U) * sizeof(float));
    buf[0] = new_data;
    for (uint8_t index = 0U; index < len; ++index)
    {
        sum += buf[index];
    }

    return sum / (float)len;
}
