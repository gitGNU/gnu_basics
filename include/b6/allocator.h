/*
 * Copyright (c) 2010, Arnaud TROEL
 * See LICENSE file for license details.
 */

#ifndef B6_ALLOCATOR_H_
#define B6_ALLOCATOR_H_

struct b6_allocator {
	const struct b6_allocator_ops *ops;
};

struct b6_allocator_ops {
	void *(*allocate)(struct b6_allocator*, unsigned long int);
	void *(*reallocate)(struct b6_allocator*, void*, unsigned long int);
	void (*deallocate)(struct b6_allocator*, void*);
};

static inline void *b6_allocate(struct b6_allocator *self,
				unsigned long int size)
{
	return self->ops->allocate(self, size);
}

static inline void *b6_reallocate(struct b6_allocator *self, void *ptr,
				  unsigned long int size)
{
	if (!ptr)
		return b6_allocate(self, size);

	if (self->ops->reallocate)
		return self->ops->reallocate(self, ptr, size);

	return NULL;
}

static inline void b6_deallocate(struct b6_allocator *self, void *ptr)
{
	self->ops->deallocate(self, ptr);
}

#endif /* B6_ALLOCATOR_H_ */
