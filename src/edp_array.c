
#include "edp_array.h"

#define MIN_CAPACITY 8

void array_init_size(Array* ar, uint32_t elemSize)
{
    ar->count       = 0;
    ar->capacity    = 0;
    ar->elemSize    = elemSize;
    ar->data        = NULL;
}

void array_deinit(Array* ar, ElemCallback dtor)
{
    if (ar->data)
    {
        uint32_t index = 0;
        void* elem;
        
        if (dtor)
        {
            while ((elem = array_get_raw(ar, index++)))
            {
                dtor(elem);
            }
        }
        
        free(ar->data);
        
        ar->count       = 0;
        ar->capacity    = 0;
        ar->data        = NULL;
    }
}

void* array_get_raw(Array* ar, uint32_t index)
{
    return (ar->count > index) ? &ar->data[index * ar->elemSize] : NULL;
}

void* array_back_raw(Array* ar)
{
    uint32_t c = ar->count;
    return (c > 0) ? &ar->data[(c - 1) * ar->elemSize] : NULL;
}

int array_set(Array* ar, uint32_t index, const void* value)
{
    uint32_t size;
    
    if (index >= ar->count)
        return ERR_Invalid;

    size = ar->elemSize;
    memcpy(&ar->data[index * size], value, size);
    
    return ERR_None;
}

static int array_realloc(Array* ar, uint32_t cap)
{
    byte* data;
    
    cap     = (cap == 0) ? MIN_CAPACITY : (cap * 2);
    data    = realloc_bytes(ar->data, cap * ar->elemSize);
    
    if (!data) return false;
    
    ar->capacity    = cap;
    ar->data        = data;
    
    return true;
}

void* array_push_back(Array* ar, const void* value)
{
    uint32_t index  = ar->count;
    uint32_t cap    = ar->capacity;
    uint32_t size   = ar->elemSize;
    void* ptr;
    
    if (index >= cap && !array_realloc(ar, cap))
        return NULL;
    
    ar->count   = index + 1;
    ptr         = &ar->data[index * size];
    
    if (value)
        memcpy(ptr, value, size);
    
    return ptr;
}

void array_pop_back(Array* ar)
{
    if (ar->count > 0)
        ar->count--;
}

int array_swap_and_pop(Array* ar, uint32_t index)
{
    if (ar->count > 0 && index < ar->count)
    {
        uint32_t back = --ar->count;
        
        if (index != back)
        {
            uint32_t size   = ar->elemSize;
            byte* data      = ar->data;
            
            memcpy(&data[index * size], &data[back * size], size);
            return true;
        }
    }
    
    return false;
}

void array_shift_left(Array* ar, uint32_t numIndices)
{
    uint32_t count  = ar->count;
    uint32_t size   = ar->elemSize;
    byte* data      = ar->data;
    
    if (count <= numIndices)
    {
        array_clear(ar);
        return;
    }
    
    if (!data) return;
    
    count -= numIndices;
    
    memmove(data, &data[numIndices * size], count * size);
    
    ar->count = count;
}

int array_reserve(Array* ar, uint32_t count)
{
    if (ar->capacity < count)
    {
        byte* data = realloc_bytes(ar->data, ar->elemSize * count);
        
        if (!data) return ERR_OutOfMemory;
        
        ar->capacity    = count;
        ar->data        = data;
    }
    
    return ERR_None;
}

void array_clear(Array* ar)
{
    ar->count = 0;
}

void array_clear_index_and_above(Array* ar, uint32_t index)
{
    if (index < ar->count)
        ar->count = index;
}

int array_append(Array* ar, const void* values, uint32_t count)
{
    uint32_t elemSize   = ar->elemSize;
    uint32_t cur        = ar->count;
    uint32_t total      = cur + count;
    int rc;
    
    rc = array_reserve(ar, total);
    
    if (rc) return rc;
    
    memcpy(ar->data + (cur * elemSize), values, count * elemSize);
    
    ar->count = total;
    
    return ERR_None;
}

static void array_sort_swap(Array* array, int a, int b, uint32_t elemSize, void* temp)
{
    void* va;
    void* vb;
    
    if (a == b)
        return;
    
    va = array_get_raw(array, a);
    vb = array_get_raw(array, b);
    
    memcpy(temp, va, elemSize);
    memcpy(va, vb, elemSize);
    memcpy(vb, temp, elemSize);
}

static int array_sort_partition(Array* array, int low, int high, uint32_t elemSize, void* temp, CmpCallback sorter)
{
    int pivotIndex = (low + high) / 2;
    int mem        = low;
    int i;
    void* vhigh;
    
    array_sort_swap(array, pivotIndex, high, elemSize, temp);
    vhigh = array_get_raw(array, high);
    
    for (i = low; i < high; i++)
    {
        if (sorter(array_get_raw(array, i), vhigh))
        {
            array_sort_swap(array, mem, i, elemSize, temp);
            mem++;
        }
    }
    
    array_sort_swap(array, mem, high, elemSize, temp);
    return mem;
}

static void array_sort_recurse(Array* array, int low, int high, uint32_t elemSize, void* temp, CmpCallback sorter)
{
    if (low < high)
    {
        int p = array_sort_partition(array, low, high, elemSize, temp, sorter);
        array_sort_recurse(array, low, p - 1, elemSize, temp, sorter);
        array_sort_recurse(array, p + 1, high, elemSize, temp, sorter);
    }
}

int array_sort(Array* ar, CmpCallback sorter)
{
    uint32_t elemSize       = ar->elemSize;
    uint32_t numElements    = ar->count;
    void* temp              = array_push_back(ar, NULL);
    
    if (!temp) return ERR_OutOfMemory;
    
    array_sort_recurse(ar, 0, numElements - 1, elemSize, temp, sorter);
    
    /* Remove the temp element */
    array_pop_back(ar);
    
    return ERR_None;
}

void array_for_each(Array* ar, ElemCallback func)
{
    uint32_t elemSize   = ar->elemSize;
    uint32_t i          = 0;
    
    for (;;)
    {
        if (i >= ar->count)
            return;
        
        func(&ar->data[i * elemSize]);
        i++;
    }
}

void array_take_ownership(Array* ar, Array* from)
{
    memcpy(ar, from, sizeof(Array));
    
    from->count     = 0;
    from->capacity  = 0;
    from->data      = NULL;
}

int array_append_array(Array* ar, const Array* src)
{
    uint32_t elemSize   = ar->elemSize;
    uint32_t ac         = ar->count;
    uint32_t sc         = src->count;
    int rc;
    
    if (elemSize != src->elemSize) return ERR_Invalid;
    
    rc = array_reserve(ar, ac + sc);
    
    if (rc) return rc;
    
    memcpy(&ar->data[ac * elemSize], src->data, sc * elemSize);
    
    return ERR_None;
}

#undef MIN_CAPACITY
