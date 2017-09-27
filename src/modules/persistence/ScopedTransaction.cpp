/**
 * @file
 */

#include "ScopedTransaction.h"
#include "Model.h"

namespace persistence {

ScopedTransaction::ScopedTransaction(Model* model, bool autocommit) :
		_autocommit(autocommit), _model(model) {
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
	_model->commit();
}

void ScopedTransaction::rollback() {
	if (_commited) {
		return;
	}
	_commited = true;
	_model->rollback();
}

}
