#ifndef MAIN_LIB_H
#define MAIN_LIB_H 1

#include "raylib.h"
#include "raymath.h"

#define FLT_MAX     340282346638528859811704183484516925440.0f

#define max(a, b) ((a)>(b)? (a) : (b))
#define min(a, b) ((a)<(b)? (a) : (b))

// Clamp Vector2 value with min and max and return a new vector2
Vector2 ClampValue(Vector2 value, Vector2 min, Vector2 max);

#endif