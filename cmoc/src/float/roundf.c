#include <coco.h>


float
roundf(float x)
{
    return truncf(x + (x >= 0 ? 0.5f : -0.5f));
}
