#pragma once


#define DI_SAUCE
//#define DI_BOOST
//#define DI_FRUIT

#ifdef DI_SAUCE
#include <sauce/sauce.h>
namespace di = sauce;
#endif

#ifdef DI_FRUIT
#include <fruit/fruit.h>
namespace di = fruit;
#endif

#ifdef DI_BOOST
#include <boost/di.hpp>
namespace di = boost::di;
namespace boost {
namespace di {
static constexpr auto SingletonScope = singleton;
template <class T, class... Ts>
using Injector = injector<T, Ts...>;
}
}
#endif
