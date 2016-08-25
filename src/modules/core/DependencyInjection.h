#pragma once


#define DI_SAUCE
//#define DI_BOOST
//#define DI_FRUIT

#ifdef DI_SAUCE
#include <sauce/sauce.h>
#endif

#ifdef DI_FRUIT
#include <fruit/fruit.h>
#endif

#ifdef DI_BOOST
#include <boost/di.hpp>
#endif
