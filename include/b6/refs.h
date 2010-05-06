/*
 * Copyright (c) 2009, Arnaud TROEL
 * See LICENSE file for license details.
 */

#ifndef B6_REFS_H_
#define B6_REFS_H_

/**
 * @file refs.h
 *
 * @brief This file provides the interface for handling with containers
 *
 * Containers are objects that allows building collections of homogeneous
 * data, so-called elements. They are of two main types. Sequential containers
 * stores elements in a raw. Associative containers stores elements in an
 * ordered manner.
 *
 * At glance, sequential containers are fast for adding and removing elements
 * but behave poorly when being searched. Associative containers require
 * elements to be comparable. They are slower for insertion and removal but
 * provide efficient search algorithms.
 *
 * Containers use references to keep track of elements. A reference is a data
 * structure that holds one or several pointer to other references.
 *
 * Elements contains a reference as a member. It is possible to dereference a
 * reference to get the pointer of the element it is bound to using
 * b6_container_of. Here is an example with a simply-linked reference:
 *
 * @code
 * struct element {
 * 	...
 * 	struct b6_sref sref;
 * 	...
 * };
 *
 * struct element *foo(struct b6_sref *ref)
 * {
 *  	return b6_container_of(ref, struct element, sref);
 * }
 * @endcode
 *
 * Note that not every reference is linked to an element: every container has
 * head and tail references that are placed before and after any reference
 * within the container respectively. It is illegal to dereference them.
 *
 * @see b6_sref, b6_dref, b6_tref
 * @see b6_ref_compare_t, b6_ref_examine_t
 */

enum { B6_NEXT, B6_PREV };

typedef int (*b6_ref_compare_t)(const void *ref1, const void *ref2);

extern int b6_default_compare(const void *a, const void *b);

/**
 * @brief Type definition for functions to be called back when searching
 * containers
 *
 * It is insured that the function is never called with the head or tail
 * reference of the queue. Thus, it is safe to dereference the reference given
 * as parameter.
 *
 * @param ref reference to check
 * @param arg opaque data specified along with this function pointer
 * @return for sequential containers, boolean value according to whether the
 * reference matches or not
 * @return for associative containers, -1 if the element is smaller than
 * expected, 1 if the element is greater than expected and 0 if the element
 * equals what is expected.
 */
typedef int (*b6_ref_examine_t)(const void *ref, void *arg);

/**
 * @ingroup deque
 * @brief Single reference
 */
struct b6_sref {
	struct b6_sref *ref; /**< pointer to the next reference */
};

/**
 * @ingroup list, splay
 * @brief Double reference
 */
struct b6_dref {
	struct b6_dref *ref[2]; /**< pointers to other references */
};

struct b6_tref {
	struct b6_tref *ref[2];
	struct b6_tref *top;
	signed char dir;
	signed char balance;
};

#endif /* B6_REFS_H_ */
