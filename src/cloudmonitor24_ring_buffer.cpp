/* 
 * cloudmonitor24_ring_buffer.cpp / CloudMonitor24 Ring Buffer implementation
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

#include <string.h>
#include "cloudmonitor24_ring_buffer.h"

#define RB_INDH(rb)                ((rb)->head)
#define RB_INDT(rb)                ((rb)->tail)

int cm24_ring_buffer_init(RINGBUFF_T *ring_buffer, void *buffer, int itemSize, int count)
{
	ring_buffer->data = buffer;
	ring_buffer->count = count;
	ring_buffer->itemSz = itemSize;
	ring_buffer->head = ring_buffer->tail = 0;
	return true;
}

int cm24_ring_buffer_push(RINGBUFF_T *ring_buffer, const void *data)
{
	uint8_t *ptr = (uint8_t *)ring_buffer->data;

	//Check for full queue
	if (cm24_ring_buffer_is_full(ring_buffer))
	{
		return false;
	}
	
	ptr += RB_INDH(ring_buffer) * ring_buffer->itemSz;
	memcpy(ptr, data, ring_buffer->itemSz);
	ring_buffer->head++;
	if(ring_buffer->head >= ring_buffer->count)
	{
		ring_buffer->head = 0;
	}
	return true;
}

int cm24_ring_buffer_pop(RINGBUFF_T *ring_buffer, void *data, uint8_t offset)
{
	uint8_t *ptr = (uint8_t *)ring_buffer->data;
	uint32_t new_tail = ring_buffer->tail;
	if(offset > 0)
	{
		new_tail+=offset;
		while( new_tail >= ring_buffer->count)
		{
			new_tail -= ring_buffer->count;
		}
	}

	//Check for empty queue
	if (ring_buffer->head == new_tail)
	{
		return false;
	}
	
	ptr += new_tail * ring_buffer->itemSz;
	memcpy(data, ptr, ring_buffer->itemSz);
	return true;
}

int cm24_ring_buffer_increment_tail(RINGBUFF_T *ring_buffer, uint8_t num_elements)
{
	ring_buffer->tail+=num_elements;
	while(ring_buffer->tail >= ring_buffer->count)
	{
		ring_buffer->tail -= ring_buffer->count;
	}
	return true;
}