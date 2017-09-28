/**
 * @file
 */

#include "ScopedTransaction.h"
#include "Model.h"

namespace persistence {

ScopedTransaction::ScopedTransaction(const DBHandlerPtr& dbHandler, bool autocommit) :
		_autocommit(autocommit), _dbHandler(dbHandler) {
	_dbHandler->begin();
}

ScopedTransaction::~ScopedTransaction() {
	if (_autocommit) {
		commit();
	} else {
		rollback();
	}
}

void ScopedTransaction::commit() {
	if (_commited) {
		return;
	}
	_commited = true;
	_dbHandler->commit();
}

void ScopedTransaction::rollback() {
	if (_commited) {
		return;
	}
	_commited = true;
	_dbHandler->rollback();
}

}
