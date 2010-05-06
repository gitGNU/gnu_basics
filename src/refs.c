#include "refs.h"

int b6_default_compare(const void *a, const void *b)
{
	return b6_sign_of((int)a - (int)b);
}
