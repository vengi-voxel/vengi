/**
 * @file
 */

#pragma once

namespace tb {

template<class CONTEXT, class TYPE>
static void insertion_sort(TYPE *elements, size_t element_count, CONTEXT context, int(*cmp)(CONTEXT context, const TYPE *a, const TYPE *b))
{
	size_t i, j;
	for (i = 1; i < element_count; i++)
	{
		TYPE value = elements[i];
		for (j = i; j > 0 && cmp(context, &value, &elements[j - 1]) < 0; j--)
			elements[j] = elements[j - 1];
		elements[j] = value;
	}
}

} // namespace tb
