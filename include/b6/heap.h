/*
 * Copyright (c) 2014, Arnaud TROEL
 * See LICENSE file for license details.
 */

/**
 * @file heap.h
 * @brief Binomial heap or priority queue.
 */

#ifndef B6_HEAP_H
#define B6_HEAP_H

#include "b6/array.h"
#include "b6/assert.h"
#include "b6/refs.h"
#include "b6/utils.h"

/**
 * @brief A heap is a container of comparable items such that the most
 * prioritary one (with regard to how items compare) can be immediately
 * acccessed.
 *
 * This implementation stores pointers to items in an array, which requires an
 * additional memory allocator.
 */
struct b6_heap {
	struct b6_array array; /**< underlying array */
	b6_compare_t compare; /**< items comparator */
	void (*set_index)(void*, unsigned long int); /**< item index callback */
};

/**
 * @internal
 */
extern void b6_heap_do_pop(struct b6_heap*);

/**
 * @internal
 */
extern void b6_heap_do_push(struct b6_heap*, void**, unsigned long int);

/**
 * @internal
 */
extern void b6_heap_do_bring_on_top(struct b6_heap*, void**, unsigned long int);

/**
 * @brief Prepare a heap for being used.
 *
 * This function must be called first or results of other functions are
 * unpredicted.
 *
 * @param self specifies the heap to initialize.
 * @param allocator specifies the allocator to use for the underlying array.
 * @param compare specifies the function to call back to compare to items so as
 * to get the most prioritary one.
 * @param set_index specifies an optional function to call back when an item is
 * assigned an index in the underlying array.
 */
static inline void b6_heap_initialize(
	struct b6_heap *self,
	struct b6_allocator *allocator,
	b6_compare_t compare,
	void (*set_index)(void*, unsigned long int))
{
	self->compare = compare;
	self->set_index = set_index;
	b6_array_initialize(&self->array, allocator, sizeof(void*));
}

/**
 * @brief Remove all items from the heap and release its resources.
 *
 * Once this function has been called, the heap cannot be used anymore until it
 * is initialized again.
 *
 * @param self specifies the heap to finalize.
 */
static inline void b6_heap_finalize(struct b6_heap *self)
{
	b6_array_finalize(&self->array);
}

/**
 * @brief Check is a heap contains any item.
 * @complexity O(1)
 * @param self specifies the heap.
 * @return how many items the heap contains.
 */
static inline int b6_heap_is_empty(const struct b6_heap *self)
{
	return !b6_array_length(&self->array);
}

/**
 * @brief Get access to the item on the top of the heap.
 * @pre The heap must not be empty.
 * @complexity O(1)
 * @param self specifies the heap.
 * @return A pointer to the top item.
 */
static inline void *b6_heap_top(const struct b6_heap *self)
{
	void **ptr = b6_array_get(&self->array, 0);
	b6_assert(!b6_heap_is_empty(self));
	return *ptr;
}

/**
 * @brief Remove the item on the top of the heap.
 * @pre The heap must not be empty.
 * @complexity O(log(n))
 * @param self specifies the heap.
 */
static inline void b6_heap_pop(struct b6_heap *self)
{
	b6_assert(!b6_heap_is_empty(self));
	b6_heap_do_pop(self);
	b6_array_reduce(&self->array, 1);
}

/**
 * @brief Insert a new item in the heap.
 * @complexity O(log(n))
 * @param self specifies the heap.
 * @param item specifies the item to insert.
 * @return 0 for success
 * @return -1 when out of memory
 */
static inline int b6_heap_push(struct b6_heap *self, void *item)
{
	unsigned long int len = b6_array_length(&self->array);
	void **ptr = b6_array_extend(&self->array, 1);
	if (!ptr)
		return -1;
	*ptr = item;
	if (self->set_index)
		self->set_index(item, len);
	b6_heap_do_push(self, b6_array_get(&self->array, 0), len);
	return 0;
}

/**
 * @brief Move an item towards the top of the heap.
 *
 * This function can be called when the priority of an item has increased so
 * that it can be moved in the heap properly.
 *
 * @see b6_heap_extract if the priority of the item has decreased.
 * @see b6_heap_initialize to get notified of the index of items.
 *
 * @complexity O(log(n))
 * @param self specifies the heap.
 * @param index specifies the index of the item to promote.
 */
static inline void b6_heap_touch(struct b6_heap *self, unsigned long int index)
{
	void **buf = b6_array_get(&self->array, 0);
	b6_assert(index < b6_array_length(&self->array));
	b6_heap_do_push(self, buf, index);
}

/**
 * @brief Removes an item from the heap.
 *
 * This function can be called to remove a specific item from the heap. Another
 * use case is when the priority of an item has decreased. This function allows
 * to remove it so as to re-insert it afterwards.
 *
 * @see b6_heap_initialize to get notified of the index of items.
 *
 * @complexity O(log(n))
 * @param self specifies the heap.
 * @param index specifies the index of the item to remove.
 */
static inline void b6_heap_extract(struct b6_heap *self,
				   unsigned long int index)
{
	void **buf = b6_array_get(&self->array, 0);
	b6_assert(index < b6_array_length(&self->array));
	b6_heap_do_bring_on_top(self, buf, index);
	b6_heap_pop(self);
}

#endif /* B6_HEAP_H */
