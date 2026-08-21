#include "ring_buffer.h"

#include <string.h>


void ring_buffer_init(struct ring_buffer *cb, uint8_t *buffer, uint8_t capacity)
{
    if (cb == NULL || buffer == NULL || capacity == 0)
    {
        return;
    }
    memset(cb, 0, sizeof(struct ring_buffer));

    cb->buffer = buffer;
    cb->capacity = capacity;
    cb->head = 0;
    cb->tail = 0;
    cb->count = 0;
}

void ring_buffer_clear(struct ring_buffer *cb)
{
    if (cb == NULL)
    {
        return;
    }

    cb->head = 0;
    cb->tail = 0;
    cb->count = 0;
}

uint16_t ring_buffer_readable(struct ring_buffer *cb)
{
    if (cb == NULL)
    {
        return 0;
    }

    return cb->count;
}

uint16_t ring_buffer_writable(struct ring_buffer *cb)
{
    if (cb == NULL)
    {
        return 0;
    }

    return cb->capacity - cb->count;
}

uint8_t ring_buffer_is_empty(struct ring_buffer *cb)
{
    if (cb == NULL)
    {
        return 1; /// 视为为空
    }

    return (cb->count == 0);
}

uint8_t ring_buffer_is_full(struct ring_buffer *cb)
{
    if (cb == NULL)
    {
        return 0; /// 视为不为满
    }

    return (cb->count == cb->capacity);
}

uint8_t ring_buffer_put(struct ring_buffer *cb, uint8_t byte)
{
    if (cb == NULL || ring_buffer_is_full(cb))
    {
        return 0;
    }

    cb->buffer[cb->tail] = byte;
    cb->tail = (cb->tail + 1) % cb->capacity;

    cb->count++;
    if (cb->count > cb->capacity)
    {
        cb->count = cb->capacity; /// 防止溢出
    }

    return 1;
}

uint8_t ring_buffer_get(struct ring_buffer *cb, uint8_t *byte)
{
    if (cb == NULL || ring_buffer_is_empty(cb))
    {
        return 0;
    }

    if (byte != NULL)
    {
        *byte = cb->buffer[cb->head];
    }

    cb->head = (cb->head + 1) % cb->capacity;
    cb->count--;

    return 1;
}