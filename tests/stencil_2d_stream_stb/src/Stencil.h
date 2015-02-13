#pragma once

#define X_SIZE 5
#define X_ORIGIN 2
#define Y_SIZE 5
#define Y_ORIGIN 3

typedef uint16_t StType_t;

struct CCoef {
	StType_t m_coef[Y_SIZE][X_SIZE];
};
