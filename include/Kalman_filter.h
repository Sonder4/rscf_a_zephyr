#ifndef KALMAN_FILTER_H_
#define KALMAN_FILTER_H_

#include "linear_algebra.h"

#define order 3
#define time_step 0.02f

typedef struct
{
    double Prior_estimation[3][1];
    double Posteriori_estimate[3][1];
    double Control_input[3][1];
    double A[3][3];
    double A_T[3][3];
    double B[3][3];
    double H[1][1];
    double H_T[1][1];
    double R[3][3];
    double Q[3][3];
    double P_Prior[3][3];
    double P_error[3][3];
    double Kk[3][3];
    double Identity_matrix[3][3];
    double Zk[3][1];
    double temp_matrix1[3][3];
    double temp_matrix2[3][3];
    double temp_matrix3[3][3];
} Parameter_init;

extern float Kalman_chassis_position_x;
extern float Kalman_chassis_position_y;
extern Parameter_init Kalman_filter_parameter_Y;
extern Parameter_init Kalman_filter_parameter_X;

void Kalman_filter_parameter_initialization(Parameter_init *Kalman_filter_parameter);
void Kalman_filter_Prior_estimation(Parameter_init *Kalman_filter_parameter);
void Kalman_filter_P_Prior(Parameter_init *Kalman_filter_parameter);
void Kalman_filter_Kk(Parameter_init *Kalman_filter_parameter);
void Kalman_filter_Posteriori_estimate(Parameter_init *Kalman_filter_parameter);
void Kalman_filter_P_error(Parameter_init *Kalman_filter_parameter);
void Kalman_filter(Parameter_init *Kalman_filter_parameter, double position, double velocity, double acc);

#endif /* KALMAN_FILTER_H_ */
