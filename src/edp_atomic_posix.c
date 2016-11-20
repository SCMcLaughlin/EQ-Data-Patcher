
#include "edp_atomic.h"

void aint32_set(aint32_t* a, int32_t val)
{
    atomic_store(a, val);
}

int32_t aint32_get(aint32_t* a)
{
    return atomic_load(a);
}

int32_t aint32_add(aint32_t* a, int32_t amt)
{
    return atomic_fetch_add(a, amt);
}

int32_t aint32_sub(aint32_t* a, int32_t amt)
{
    return atomic_fetch_sub(a, amt);
}

int aint32_cmp_xchg_weak(aint32_t* a, int32_t expected, int32_t desired)
{
    return atomic_compare_exchange_weak(a, &expected, desired);
}

int aint32_cmp_xchg_strong(aint32_t* a, int32_t expected, int32_t desired)
{
    return atomic_compare_exchange_strong(a, &expected, desired);
}

void aint16_set(aint16_t* a, int16_t val)
{
    atomic_store(a, val);
}

int16_t aint16_get(aint16_t* a)
{
    return atomic_load(a);
}

int aint16_cmp_xchg_weak(aint16_t* a, int16_t expected, int16_t desired)
{
    return atomic_compare_exchange_weak(a, &expected, desired);
}

int aint16_cmp_xchg_strong(aint16_t* a, int16_t expected, int16_t desired)
{
    return atomic_compare_exchange_strong(a, &expected, desired);
}

void amutex_unlock(amutex* mtx)
{
    atomic_flag_clear(mtx);
}

void amutex_lock(amutex* mtx)
{
    for (;;)
    {
        if (amutex_try_lock(mtx))
            break;
    }
}

int amutex_try_lock(amutex* mtx)
{
    return atomic_flag_test_and_set(mtx) == false;
}
