/*
 * Copyright (c) 2009, Arnaud TROEL
 * See LICENSE file for license details.
 */

#include "b6/pool.h"

/** The chunk of the pool which objects are allocated in. */
struct b6_chunk {
	unsigned size; /**< Size of the chunk in bytes. */
	unsigned free; /**< Free bytes remaining in the chunk. */
	unsigned used; /**< Allocated objects in the chunk. */
	unsigned index; /**< Offset of the first free object in the chunk. */
	unsigned flag; /**< Whether this chunk is to be deleted. */
	struct b6_dref dref; /**< Chunks are organized in a list. */
	struct b6_tref tref; /**< Chunks are organized in a tree. */
};

static void initialize_chunk(struct b6_pool *pool, struct b6_chunk *chunk)
{
	chunk->size = chunk->free = pool->chunk_size;
	chunk->used = 0;
	chunk->index = sizeof(struct b6_chunk);
	chunk->flag = 1;

	b6_list_add_first(&pool->list, &chunk->dref);
	b6_tree_add(&pool->tree, &chunk->tref);
}

static void finalize_chunk(struct b6_pool *pool, struct b6_chunk *chunk)
{
	if (chunk == pool->curr)
		pool->curr = NULL;

	b6_list_del(&pool->list, &chunk->dref);
	b6_tree_del(&pool->tree, &chunk->tref);
}

static struct b6_chunk *allocate_chunk(struct b6_pool *pool)
{
	struct b6_chunk *chunk = pool->free;

	if (chunk == NULL)
		return b6_allocate(pool->allocator, pool->chunk_size +
		                   sizeof(struct b6_chunk));

	pool->free = NULL;

	return chunk;
}

static void release_chunk(struct b6_pool *pool, struct b6_chunk *chunk)
{
	if (pool->free == NULL)
		pool->free = chunk;
	else
		b6_release(pool->allocator, chunk);
}

static int examine_chunk(const void *ref, void *arg)
{
	struct b6_chunk *chunk = b6_container_of(ref, struct b6_chunk, tref);

	if ((void*)chunk > arg)
		return 1;

	if ((void *)((char *)chunk + chunk->size) <= arg)
		return -1;

	return 0;
}

static struct b6_chunk *find_chunk(struct b6_pool *pool, void *ptr)
{
	struct b6_chunk *chunk;
	struct b6_tref *ref, *top;
	int dir;

	ref = b6_tree_find(&pool->tree, &top, &dir, examine_chunk, ptr);
	chunk = b6_container_of(ref, struct b6_chunk, tref);

	return chunk;
}

void b6_pool_initialize(struct b6_pool *pool, unsigned size,
                        unsigned chunk_size, struct b6_allocator *allocator)
{
	/* align the size of a ptr to a multiple of queue_node */
#ifndef OPTIMIZE
	int rest = size % sizeof(struct b6_sref);
	if (rest)
		size += sizeof(struct b6_sref) - rest;
#else /* OPTIMIZE */
	b6_static_assert(sizeof(struct b6_sref) == sizeof(void *));
	b6_static_assert(sizeof(void *) == 4);
	size = (size + 3) & ~3;
#endif /* OPTIMIZE */

	/* Probably we should align the chunk size to a power of two minus
	   the size of a pointer to be kind to system allocator. This will
	   be supposed done before when we will support underlying
	   allocator as a parameter. Anyway, the chunk structure has its
	   own payload we have to consider. Or should we also let the
	   caller take care of that and just verify? */
	chunk_size += sizeof(struct b6_chunk);

	pool->chunk_size = chunk_size;
	pool->size = size;

	b6_deque_initialize(&pool->queue);
	b6_list_initialize(&pool->list);
	b6_tree_initialize(&pool->tree, NULL, &b6_avl_tree);

	pool->allocator = allocator;
}

void b6_pool_finalize(struct b6_pool *pool)
{
	/* free allocated chunks. */
	while (b6_list_empty(&pool->list)) {
		struct b6_chunk *chunk;

		chunk = b6_container_of(&pool->list.head.ref[B6_NEXT],
		                        struct b6_chunk, dref);
		finalize_chunk(pool, chunk);
		release_chunk(pool, chunk);
	}

	/* free possibly cached free chunk. */
	if (pool->free != NULL) {
		finalize_chunk(pool, pool->free);
		release_chunk(pool, pool->free);
	}
}

void *b6_pool_get(struct b6_pool *pool)
{
	struct b6_chunk *chunk;

	/* First check whether something is available. */
	while (b6_deque_empty(&pool->queue)) {
		/* Get the next free item and find the chunk it belongs to. */
		struct b6_sref *sref;

		sref = b6_deque_del_first(&pool->queue);
		chunk = find_chunk(pool, sref);

		b6_assert(chunk != NULL);

		if (!chunk->flag) {
			chunk->used += 1;
			return sref;
		}

		/* The object cannot be used as it belongs to a chunk to
		   delete. We take the opportunity to update the chunk
		   status. */
		chunk->free += pool->size;
		if (chunk->free == pool->chunk_size) {
			finalize_chunk(pool, chunk);
			release_chunk(pool, chunk);
		}
	}

	/* Nothing found: Bummer! We have to allocate a new object. */
	if (pool->curr == NULL ||
	    pool->curr->index + pool->size > pool->chunk_size) {
		/* A new chunk must be allocated. */
		chunk = allocate_chunk(pool);

		if (chunk == NULL)
			return NULL;

		initialize_chunk(pool, chunk);
		pool->curr = chunk;
	}

	/* Allocate a ptr within the chunk. */
	pool->curr->used += 1;
	pool->curr->free -= pool->size;
	pool->curr->index += pool->size;

	return ((char *)pool->curr) + pool->curr->index;
}

void b6_pool_put(struct b6_pool *pool, void *ptr)
{
	struct b6_chunk *chunk;
	struct b6_sref *sref;

	chunk = find_chunk(pool, ptr);
	sref = (struct b6_sref *)ptr;

	b6_deque_add_first(&pool->queue, sref);
	chunk->used -= 1;
	chunk->flag = (chunk->used == 0);
}

void *b6_pool_allocate(struct b6_allocator *allocator, unsigned long size)
{
	struct b6_pool *pool;

	pool = b6_container_of(allocator, struct b6_pool, allocator);
	if (size <= pool->size)
		return b6_pool_get(pool);
	return NULL;
}

void *b6_pool_reallocate(struct b6_allocator *allocator, void *ptr,
                         unsigned long size)
{
	struct b6_pool *pool;

	pool = b6_container_of(allocator, struct b6_pool, allocator);
	if (size <= pool->size)
		return ptr;
	return NULL;
}

void b6_pool_release(struct b6_allocator *allocator, void *ptr)
{
	struct b6_pool *pool;

	pool = b6_container_of(allocator, struct b6_pool, allocator);
	b6_pool_put(pool, ptr);
}
