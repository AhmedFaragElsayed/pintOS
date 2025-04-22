#ifndef THREADS_FIXED_POINT_H
#define THREADS_FIXED_POINT_H


typedef int fixed_point;
typedef signed long int64_t;

#define EXPONENT 1 << 14

// int -> fixed_point
#define CONVERT_TO_FIXED_POINT(integer) integer*EXPONENT

// fixed_point -> int
#define CONVERT_TO_INT(fixedpoint) fixedpoint / EXPONENT

// round(fixed_point) -> int
#define CONVERT_TO_INT_ROUNDING(fixedpoint)   (fixedpoint  + (fixedpoint>=0? EXPONENT : -EXPONENT) >>1 )/EXPONENT

// Adds two fixed points -> fixed_point
#define ADD_FIXED_POINTS(fixedpoint_1  , fixedpoint_2) fixedpoint_1 + fixedpoint_2

// Subtracts two fixed points -> fixed_point
#define SUBTRACT_FIXED_POINTS(fixedpoint_1  , fixedpoint_2) fixedpoint_1 - fixedpoint_2

// Adds a fixed_point to an int -> fixed_point
#define ADD_FIXED_POINT_TO_INT(fixedpoint , integer) fixedpoint + CONVERT_TO_FIXED_POINT(integer)

// Subtracts an int from a fixed_point  -> fixed_point
#define SUBTRACT_INT_FROM_FIXED_POINT(fixedpoint , integer) fixedpoint - CONVERT_TO_FIXED_POINT(integer)

// Subtracts a fixed_point from an int  -> int
#define SUBTRACT_FIXED_POINT_FROM_INT(fixedpoint , integer) CONVERT_TO_FIXED_POINT(integer) - fixedpoint

// Multiplies two fixed_points -> int64_t
#define MULTIPLY_FIXED_POINTS(fixedpoint_1 , fixedpoint_2) (int64_t)fixedpoint_1 * CONVERT_TO_INT(fixedpoint_2)

// Multiplies a fixed_point with an int -> int
#define MULTIPLY_INT_BY_FIXED_POINT(fixedpoint , integer) fixedpoint * integer

// Divides a fixed_point by a fixed_point -> int64_t
#define DIVIDE_FIXED_POINTS(fixedpoint_1 , fixedpoint_2) (int64_t)fixedpoint_1 * (EXPONENT/fixedpoint_2)

// Divides a fixed_point by an int -> fixed_point
#define DIVIDE_FIXED_POINT_BY_INT(fixedpoint , integer) fixedpoint / integer





#endif