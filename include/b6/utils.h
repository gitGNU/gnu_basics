/*
 * Copyright (c) 2009, Arnaud TROEL
 * See LICENSE file for license details.
 */

#ifndef UTILS_H_
#define UTILS_H_

#ifndef NULL
#define NULL ((void *)0L)
#endif /* NULL */

/**
 * Return the offset of a field within a structure.
 * @param t specifies the type.
 * @param f specifies the name of the field.
 * @return the offset of the field within the type.
 */
#define b6_offset_of(t, f) (int) &(((t *) 0)->f)

/**
 * Cast a field of a structure out to the containing structure
 * @param p specifies the pointer
 * @param t specifies the type that contains f
 * @param f specifies the field referenced by p
 * @return a pointer to the variable of type t
 */
#define b6_container_of(p, t, f) (t *) (((char *) p) - b6_offset_of(t, f))

/**
 * Get the cardinality of an array.
 * @param a specifies the array.
 * @return the number of elements of the array.
 */
#define b6_card_of(a) (sizeof(a) / sizeof(a[0]))

/**
 * Calculates the sign of a signed integer.
 * @param i signed integer
 * @retval -1 if i is negative strictly,
 * @retval  0 if i equals zero,
 * @retval  1 if i is positive strictly.
 */
#define b6_sign_of(i)                                                         \
({                                                                            \
	__typeof(i) _i = (i);                                                 \
	((_i) >> (sizeof(_i) * 8 - 1)) - ((-(_i)) >> (sizeof(_i) * 8 - 1));   \
})

/**
 * @def b6_likely
 * Specifies the underlying condition is supposed to be true most of
 *        the time and instruct the compiler to optimize the code accordingly.
 * @param x specifies the condition
 * @note This feature is compiler dependent and might have no effect.
 */

/**
 * @def b6_unlikely
 * Specifies the underlying condition is supposed to be false most of
 *        the time and instruct the compiler to optimize the code accordingly.
 * @param x specifies the condition
 * @note This feature is compiler dependent and might have no effect.
 */

#if (defined (__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__ >= 296))
#define b6_likely(x)   __builtin_expect(!!(x),1)
#define b6_unlikely(x) __builtin_expect(!!(x),0)
#else
#define b6_likely(x)   x
#define b6_unlikely(x) x
#endif

/**
 * Declare a local variable or a function parameter as not used.
 *
 * This facility is particularly useful in callback function where not every
 * parameter is used, so as to eliminate compiler warnings.
 *
 * Once stated unused, the variable must not be used.
 *
 * @param x specifies the variable.
 */
#define b6_unused(x) (x)=(x)

/**
 * @def __pure
 * Specifies the function behaves as a mathematical function.
 *
 * @warning In particular, it means that the function cannot have side-effects
 * or rely on any global variable. If not, do not tag the function as pure or
 * unexpected results may occur.
 *
 * @note This feature is compiler dependent and might have no effect.
 */

#if (defined (__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__ >= 296))
#define b6_pure __attribute__ ((pure))
#else
#define b6_pure
#endif

#endif /*UTILS_H_*/