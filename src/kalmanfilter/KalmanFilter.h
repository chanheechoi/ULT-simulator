#pragma once
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_linalg.h>
#include "Vector.h"

#define KF_X	0
#define KF_X0	1
#define KF_A	2
#define KF_B	3
#define KF_U	4
#define KF_Q	5
#define KF_H	6
#define KF_R	7
#define KF_P	8
#define KF_P0	9
#define	N_KF	10

class KalmanFilter
{
public:
	KalmanFilter(void);
	KalmanFilter(int nX, int nU, int nZ);
	KalmanFilter(KalmanFilter *kf);
	~KalmanFilter(void);

	void predict();
	void correct(gsl_matrix *Z);

	void fast_predict();
	void fast_correct(gsl_matrix *Z);	

	void SetMatrix(int mIdx, gsl_matrix *m);

	void Duplicate(KalmanFilter &kFilter);

	void matrix_alloc(int nX, int nU, int nZ);
	void matrix_free();

	void PrintMatrix(char* m_name, gsl_matrix *matrix);

//	double GetError();
//	Vector GetX0();
//	double GetX();

	double error;
	double pdf;
	
	
public:

	gsl_matrix *m[N_KF];

	Vector vMeasurement;
};
