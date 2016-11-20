
#ifndef EDP_ATOMIC_H
#define EDP_ATOMIC_H

#include "define.h"
#include "structs.h"

void aint32_set(aint32_t* a, int32_t val);
int32_t aint32_get(aint32_t* a);
int32_t aint32_add(aint32_t* a, int32_t amt);
int32_t aint32_sub(aint32_t* a, int32_t amt);
int aint32_cmp_xchg_weak(aint32_t* a, int32_t expected, int32_t desired);
int aint32_cmp_xchg_strong(aint32_t* a, int32_t expected, int32_t desired);

void aint16_set(aint16_t* a, int16_t val);
int16_t aint16_get(aint16_t* a);
int aint16_cmp_xchg_weak(aint16_t* a, int16_t expected, int16_t desired);
int aint16_cmp_xchg_strong(aint16_t* a, int16_t expected, int16_t desired);

void amutex_unlock(amutex* mtx);
void amutex_lock(amutex* mtx);
int amutex_try_lock(amutex* mtx);
#define amutex_init(mtx) amutex_unlock((mtx))

#endif/*EDP_ATOMIC_H*/
