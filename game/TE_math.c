#include "TE_math.h"

float fLerp(float a, float b, float t)
{
    return a + (b - a) * t;
}

float fLerpClamped(float a, float b, float t)
{
    return fLerp(a, b, t < 0.0f ? 0.0f : t > 1.0f ? 1.0f
                                                  : t);
}

float fMoveTowards(float current, float target, float maxDelta)
{
    float delta = target - current;
    if (delta > maxDelta)
        return current + maxDelta;
    if (delta < -maxDelta)
        return current - maxDelta;
    return target;
}

float fTweenElasticOut(float t)
{
    const float c4 = (2.0f * TE_PI) / 3.0f;

    return t <= 0.0f
               ? 0.0f
           : t >= 1.0f
               ? 1.0f
               : powf(2.0f, -10.0f * t) * sinf((t * 5.0f - 0.75f) * c4) + 1.0f;
}

int absi(int a)
{
    return a < 0 ? -a : a;
}


int16_t max_s16(int16_t a, int16_t b)
{
    return a > b ? a : b;
}
int16_t min_s16(int16_t a, int16_t b)
{
    return a < b ? a : b;
}

float max_f(float a, float b)
{
    return a > b ? a : b;
}

float min_f(float a, float b)
{
    return a < b ? a : b;
}

float sign_f(float a)
{
    return a < 0.0f ? -1.0f : (a > 0.0f ? 1.0f : 0.0f);
}
