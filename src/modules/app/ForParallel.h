/**
 * @file
 */

#pragma once

namespace app {

using ForParallelFunc = void (*)(void *, int, int);

void for_not_parallel_impl(int start, int end, ForParallelFunc fn, void *ctx);
void for_parallel_impl(int start, int end, ForParallelFunc fn, void *ctx, bool wait);
int for_parallel_size(int start, int end);

template<typename F>
inline void for_not_parallel(int start, int end, const F &f) {
	for_not_parallel_impl(
		start, end, [](void *ctx, int s, int e) { (*(const F *)ctx)(s, e); }, (void *)&f);
}

template<typename F>
inline void for_parallel(int start, int end, const F &f, bool wait = true) {
	for_parallel_impl(
		start, end, [](void *ctx, int s, int e) { (*(const F *)ctx)(s, e); }, (void *)&f, wait);
}

} // namespace app
