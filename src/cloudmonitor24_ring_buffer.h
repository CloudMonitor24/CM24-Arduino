/* 
 * cloudmonitor24_ring_buffer.h / CloudMonitor24 Platform Protocol implementation
 *
 * Copyright (c) 2016  Federico Mosca <federico.mosca@imotion.it>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef __RING_BUFFER_H_
#define __RING_BUFFER_H_

#include <stdint.h>

typedef struct
{
	void *data;
	int count;
	int itemSz;
	uint32_t head;
	uint32_t tail;
} RINGBUFF_T;

#define RB_VHEAD(rb)              (*(volatile uint32_t *) &(rb)->head)
#define RB_VTAIL(rb)              (*(volatile uint32_t *) &(rb)->tail)


int cm24_ring_buffer_init(RINGBUFF_T *ring_buffer, void *buffer, int itemSize, int count);

static inline int cm24_ring_buffer_get_count(RINGBUFF_T *ring_buffer)
{
	return RB_VHEAD(ring_buffer) - RB_VTAIL(ring_buffer);
}

static inline int cm24_ring_buffer_is_full(RINGBUFF_T *ring_buffer)
{
	return (cm24_ring_buffer_get_count(ring_buffer) >= ring_buffer->count);
}

int cm24_ring_buffer_push(RINGBUFF_T *ring_buffer, const void *data);

int cm24_ring_buffer_pop(RINGBUFF_T *ring_buffer, void *data, uint8_t offset);

int cm24_ring_buffer_increment_tail(RINGBUFF_T *ring_buffer,uint8_t num_elements);

#endif
