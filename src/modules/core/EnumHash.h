#pragma once

#if 0
namespace std {
template<class E> class hash {
	using sfinae = typename std::enable_if<std::is_enum<E>::value, E>::type;
public:
	size_t operator()(const E&e) const {
		return std::hash<typename std::underlying_type<E>::type>()(e);
	}
};
}
#endif

namespace core {

template<class T>
inline constexpr auto enumValue(T enumConstant) {
	return static_cast<typename std::underlying_type<T>::type>(enumConstant);
}

}
