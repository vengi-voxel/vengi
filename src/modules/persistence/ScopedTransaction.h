/**
 * @file
 */

#include "ForwardDecl.h"
#include "DBHandler.h"

namespace persistence {

class ScopedTransaction {
private:
	bool _commited = false;
	bool _autocommit;
	DBHandlerPtr _dbHandler;
public:
	ScopedTransaction(const DBHandlerPtr& model, bool autocommit = true);
	~ScopedTransaction();

	void commit();
	void rollback();
};

}
