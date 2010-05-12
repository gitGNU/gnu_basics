/*
 * Copyright (c) 2010, Arnaud TROEL
 * See LICENSE file for license details.
 */

#include "b6/vector.h"
#include "b6/allocator.h"

static void *b6_vector_expand(const struct b6_vector *self, unsigned long int n)
{
	unsigned long int capacity;
	unsigned long long int size;
	void *buffer;

	for (capacity = self->capacity ? self->capacity * 2 : 2;
	     n >= capacity - self->length; capacity *= 2)
		if (capacity < self->capacity)
			return NULL;

	size = (unsigned long long int)self->itemsize * capacity;
	if (size > ~0UL)
		return NULL;

	buffer = b6_reallocate(self->allocator, self->buffer, size);
	if (buffer) {
		((struct b6_vector*)self)->buffer = buffer;
		((struct b6_vector*)self)->capacity = capacity;
	}

	return buffer;
}

void *__b6_vector_add(struct b6_vector *self, unsigned long int index,
		      unsigned long int n)
{
	unsigned long long int r;
	unsigned long int m, s, d;
	char *ptr;

	if (b6_unlikely(index > self->length))
		index = self->length;

	r = (unsigned long long int)self->itemsize * index;
	if (r > ~0UL)
		return NULL;
	s = r;

	if (!n)
		return &self->buffer[s];

	r = (unsigned long long int)self->itemsize * n;
	if (r > ~0UL)
		return NULL;
	d = r;

	m = self->length + n;
	if (m < self->length)
		return NULL;

	if (m > self->capacity && !b6_vector_expand(self, n))
		return NULL;

	ptr = &self->buffer[s];
	m = self->length - index;
	if (m)
		self->ops->move(self, ptr + d, ptr, m);

	self->length += n;

	return ptr;
}

unsigned long int __b6_vector_del(struct b6_vector *self,
				  unsigned long int index, unsigned long int n)
{
	unsigned long long int r;
	unsigned long int m, s, d;
	char *ptr;

	if (!n)
		return 0;

	if (index >= self->length)
		return 0;

	m = self->length - index;
	if (n >= m) {
		n = m;
		goto job_done;
	}

	r = (unsigned long long int)self->itemsize * index;
	if (r > ~0UL)
		return 0;
	s = r;

	r = (unsigned long long int)self->itemsize * n;
	if (r > ~0UL)
		return 0;
	d = r;

	ptr = &self->buffer[s];
	self->ops->move(self, ptr + d, ptr, m);

job_done:
	self->length -= n;

	return n;
}
