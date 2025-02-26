/**
 * @file
 */

#include <stdint.h>

#include <SDL_version.h>

#if SDL_VERSION_ATLEAST(3, 2, 0)
struct SDL_Semaphore;
using core_semaphore = SDL_Semaphore;
#else
struct SDL_semaphore;
using core_semaphore = SDL_semaphore;
#endif

namespace core {

enum class SemaphoreWaitState { Success, WouldBlock, Error };

/**
 * @note Each @c waitAndDecrease() operation will atomically decrement the value and potentially block if the semaphore
 * value is 0. Each @c increase() operation will atomically increment the value and wake potentially waiting threads.
 */
class Semaphore {
private:
	core_semaphore *_semaphore;

public:
	/**
	 * @brief Construct a new Semaphore object
	 *
	 * @param initialValue The initial value defines how many times the code can pass through the semaphore before the
	 * lock is held.
	 */
	Semaphore(uint32_t initialValue = 1);
	~Semaphore();

	/**
	 * @brief This function suspends the calling thread until the semaphore has a positive count. It then atomically
	 * decreases the semaphore count.
	 * @return false on error
	 */
	bool waitAndDecrease();
	/**
	 * @brief Atomically increases the semaphore's count (not blocking).
	 * @return false on error
	 */
	bool increase();
	SemaphoreWaitState tryWait();
	bool waitTimeout(uint32_t timeout);
	/**
	 * @brief Get the current value of the semaphore.
	 */
	uint32_t value();
};

} // namespace core
