/**
 * @file
 */

#pragma once

#include "Model.h"

namespace persistence {

struct Range {
	int limit;
	int offset;
};

/**
 * @brief Value class that contains the order, limit and offset data to build a sql statement.
 *
 * @ingroup Persistence
 */
class OrderBy {
public:
	constexpr OrderBy(const FieldName _fieldname, Order _order = Order::ASC, int _limit = -1, int _offset = -1) :
			fieldname(_fieldname), order(_order), range{_limit, _offset} {
	}
	const FieldName fieldname;
	const Order order;
	const Range range;
};

}
