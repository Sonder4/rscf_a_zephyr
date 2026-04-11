#include <stdint.h>
#include <string.h>

#include "Kalman_filter.h"

float Kalman_chassis_position_x = 0.0f;
float Kalman_chassis_position_y = 0.0f;
Parameter_init Kalman_filter_parameter_X;
Parameter_init Kalman_filter_parameter_Y;

void Kalman_filter_parameter_initialization(Parameter_init *Kalman_filter_parameter)
{
    if (Kalman_filter_parameter == NULL)
    {
        return;
    }

    memset(Kalman_filter_parameter, 0, sizeof(Parameter_init));
    Kalman_filter_parameter->A[0][0] = 1.0;
    Kalman_filter_parameter->A[0][1] = time_step;
    Kalman_filter_parameter->A[0][2] = time_step * time_step * 0.5;
    Kalman_filter_parameter->A[1][1] = 1.0;
    Kalman_filter_parameter->A[1][2] = time_step;
    Kalman_filter_parameter->A[2][2] = 1.0;
    Kalman_filter_parameter->A_T[0][0] = 1.0;
    Kalman_filter_parameter->A_T[1][0] = time_step;
    Kalman_filter_parameter->A_T[2][0] = time_step * time_step * 0.5;
    Kalman_filter_parameter->A_T[1][1] = 1.0;
    Kalman_filter_parameter->A_T[2][1] = time_step;
    Kalman_filter_parameter->A_T[2][2] = 1.0;

    for (uint8_t index = 0U; index < 3U; ++index)
    {
        Kalman_filter_parameter->Identity_matrix[index][index] = 1.0;
        Kalman_filter_parameter->Q[index][index] = 0.01;
        Kalman_filter_parameter->R[index][index] = 0.1;
        Kalman_filter_parameter->P_error[index][index] = 1.0;
        Kalman_filter_parameter->H[0][0] = 1.0;
        Kalman_filter_parameter->H_T[0][0] = 1.0;
    }
}

void Kalman_filter_Prior_estimation(Parameter_init *Kalman_filter_parameter)
{
    if (Kalman_filter_parameter == NULL)
    {
        return;
    }

    matrix_multiplication(&(Kalman_filter_parameter->A[0][0]),
                          3,
                          3,
                          &(Kalman_filter_parameter->Posteriori_estimate[0][0]),
                          3,
                          1,
                          &(Kalman_filter_parameter->Prior_estimation[0][0]));
}

void Kalman_filter_P_Prior(Parameter_init *Kalman_filter_parameter)
{
    if (Kalman_filter_parameter == NULL)
    {
        return;
    }

    matrix_multiplication(&(Kalman_filter_parameter->A[0][0]),
                          3,
                          3,
                          &(Kalman_filter_parameter->P_error[0][0]),
                          3,
                          3,
                          &(Kalman_filter_parameter->temp_matrix1[0][0]));
    matrix_multiplication(&(Kalman_filter_parameter->temp_matrix1[0][0]),
                          3,
                          3,
                          &(Kalman_filter_parameter->A_T[0][0]),
                          3,
                          3,
                          &(Kalman_filter_parameter->P_Prior[0][0]));
    matrix_addition(&(Kalman_filter_parameter->P_Prior[0][0]),
                    3,
                    3,
                    1,
                    &(Kalman_filter_parameter->Q[0][0]),
                    3,
                    3,
                    1,
                    &(Kalman_filter_parameter->P_Prior[0][0]));
}

void Kalman_filter_Kk(Parameter_init *Kalman_filter_parameter)
{
    double inverse[9] = {0.0};
    double identity[9] = {0.0};

    if (Kalman_filter_parameter == NULL)
    {
        return;
    }

    matrix_inversion(&(Kalman_filter_parameter->P_Prior[0][0]), inverse, identity, 3);
    memcpy(Kalman_filter_parameter->Kk, inverse, sizeof(Kalman_filter_parameter->Kk));
}

void Kalman_filter_Posteriori_estimate(Parameter_init *Kalman_filter_parameter)
{
    if (Kalman_filter_parameter == NULL)
    {
        return;
    }

    matrix_addition(&(Kalman_filter_parameter->Prior_estimation[0][0]),
                    3,
                    1,
                    1,
                    &(Kalman_filter_parameter->Zk[0][0]),
                    3,
                    1,
                    1,
                    &(Kalman_filter_parameter->Posteriori_estimate[0][0]));
}

void Kalman_filter_P_error(Parameter_init *Kalman_filter_parameter)
{
    if (Kalman_filter_parameter == NULL)
    {
        return;
    }

    memcpy(Kalman_filter_parameter->P_error,
           Kalman_filter_parameter->P_Prior,
           sizeof(Kalman_filter_parameter->P_error));
}

void Kalman_filter(Parameter_init *Kalman_filter_parameter, double position, double velocity, double acc)
{
    if (Kalman_filter_parameter == NULL)
    {
        return;
    }

    Kalman_filter_parameter->Zk[0][0] = position;
    Kalman_filter_parameter->Zk[1][0] = velocity;
    Kalman_filter_parameter->Zk[2][0] = acc;
    Kalman_filter_Prior_estimation(Kalman_filter_parameter);
    Kalman_filter_P_Prior(Kalman_filter_parameter);
    Kalman_filter_Kk(Kalman_filter_parameter);
    Kalman_filter_Posteriori_estimate(Kalman_filter_parameter);
    Kalman_filter_P_error(Kalman_filter_parameter);
}
