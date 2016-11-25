
#ifndef EDP_ARRAY_H
#define EDP_ARRAY_H

#include "define.h"
#include "structs.h"
#include "edp_alloc.h"

void array_init_size(Array* ar, uint32_t elemSize);
#define array_init(ar, type) array_init_size((ar), sizeof(type))
void array_deinit(Array* ar, ElemCallback dtor);

#define array_count(ar) ((ar)->count)
#define array_capacity(ar) ((ar)->capacity)
#define array_elem_size(ar) ((ar)->elemSize)
#define array_raw(ar) ((ar)->data)
#define array_data(ar, type) (type*)array_raw(ar)
#define array_empty(ar) (array_count((ar)) == 0)
#define array_is_empty(ar) array_empty(ar)

void* array_get_raw(Array* ar, uint32_t index);
#define array_get(ar, index, type) (type*)array_get_raw((ar), (index))
void* array_back_raw(Array* ar);
#define array_back(ar, type) (type*)array_back_raw((ar))

int array_set(Array* ar, uint32_t index, const void* value);
void* array_push_back(Array* ar, const void* value);
#define array_push_back_type(ar, value, type) (type*)array_push_back((ar), (value))
void array_pop_back(Array* ar);
int array_swap_and_pop(Array* ar, uint32_t index);
void array_shift_left(Array* ar, uint32_t numIndices);
int array_reserve(Array* ar, uint32_t count);
void array_clear(Array* ar);
void array_clear_index_and_above(Array* ar, uint32_t index);

int array_append(Array* ar, const void* values, uint32_t count);
int array_sort(Array* ar, CmpCallback sorter);

void array_for_each(Array* ar, ElemCallback func);

void array_take_ownership(Array* ar, Array* from);
int array_append_array(Array* ar, const Array* src);

#endif/*EDP_ARRAY_H*/
