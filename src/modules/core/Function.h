/**
 * @file
 */

#pragma once

#include "core/Common.h"
#include "core/StandardLib.h"

namespace core {

template<typename>
class Function;

/**
 * @brief A lightweight replacement for std::function that avoids STL dependencies.
 *
 * Uses a small buffer optimization (SBO) to avoid heap allocation for small callables.
 * Falls back to heap allocation for larger callables.
 */
template<typename Ret, typename... Args>
class Function<Ret(Args...)> {
	static constexpr int SBO_SIZE = sizeof(void *) * 4;

	struct CallableBase {
		virtual ~CallableBase() = default;
		virtual Ret invoke(Args... args) = 0;
		virtual void copyTo(void *dst) const = 0;
		virtual void moveTo(void *dst) = 0;
		virtual int size() const = 0;
	};

	template<typename F>
	struct Callable final : CallableBase {
		F _f;
		Callable(const F &f) : _f(f) {}
		Callable(F &&f) : _f(core::move(f)) {}
		Ret invoke(Args... args) override { return _f(args...); }
		void copyTo(void *dst) const override { new (dst) Callable(_f); }
		void moveTo(void *dst) override { new (dst) Callable(core::move(_f)); }
		int size() const override { return (int)sizeof(Callable); }
	};

	alignas(alignof(void *)) char _sbo[SBO_SIZE];
	CallableBase *_callable = nullptr;

	bool isLocal() const {
		const char *p = (const char *)_callable;
		return p >= _sbo && p < _sbo + SBO_SIZE;
	}

	void destroy() {
		if (_callable) {
			if (isLocal()) {
				_callable->~CallableBase();
			} else {
				_callable->~CallableBase();
				core_free(_callable);
			}
			_callable = nullptr;
		}
	}

	template<typename F>
	void assign(F &&f) {
		using CallableType = Callable<typename remove_reference<F>::type>;
		if constexpr (sizeof(CallableType) <= SBO_SIZE) {
			_callable = new (_sbo) CallableType(core::forward<F>(f));
		} else {
			void *mem = core_malloc(sizeof(CallableType));
			_callable = new (mem) CallableType(core::forward<F>(f));
		}
	}

public:
	Function() = default;

	Function(decltype(nullptr)) {}

	template<typename F, typename = typename core::enable_if<!core::is_same<typename core::decay<F>::type, Function>::value>::type>
	Function(F &&f) { // NOLINT(bugprone-forwarding-reference-overload)
		assign(core::forward<F>(f));
	}

	Function(const Function &other) {
		if (other._callable) {
			if (other.isLocal()) {
				other._callable->copyTo(_sbo);
				_callable = (CallableBase *)_sbo;
			} else {
				void *mem = core_malloc(other._callable->size());
				other._callable->copyTo(mem);
				_callable = (CallableBase *)mem;
			}
		}
	}

	Function(Function &&other) noexcept {
		if (other._callable) {
			if (other.isLocal()) {
				other._callable->moveTo(_sbo);
				_callable = (CallableBase *)_sbo;
				other._callable->~CallableBase();
			} else {
				_callable = other._callable;
			}
			other._callable = nullptr;
		}
	}

	~Function() {
		destroy();
	}

	Function &operator=(const Function &other) {
		if (this != &other) {
			destroy();
			if (other._callable) {
				if (other.isLocal()) {
					other._callable->copyTo(_sbo);
					_callable = (CallableBase *)_sbo;
				} else {
					void *mem = core_malloc(other._callable->size());
					other._callable->copyTo(mem);
					_callable = (CallableBase *)mem;
				}
			}
		}
		return *this;
	}

	Function &operator=(Function &&other) noexcept {
		if (this != &other) {
			destroy();
			if (other._callable) {
				if (other.isLocal()) {
					other._callable->moveTo(_sbo);
					_callable = (CallableBase *)_sbo;
					other._callable->~CallableBase();
				} else {
					_callable = other._callable;
				}
				other._callable = nullptr;
			}
		}
		return *this;
	}

	Function &operator=(decltype(nullptr)) {
		destroy();
		return *this;
	}

	Ret operator()(Args... args) const {
		return _callable->invoke(args...);
	}

	explicit operator bool() const {
		return _callable != nullptr;
	}
};

} // namespace core
