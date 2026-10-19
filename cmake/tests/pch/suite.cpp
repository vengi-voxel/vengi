#include <gtest/gtest.h>
#include <SDL3/SDL.h>

int main() {
	static_assert(GTEST_HAS_PTHREAD == 0 || GTEST_HAS_PTHREAD == 1, "gtest configuration");
	return SDL_MAJOR_VERSION != 3;
}
