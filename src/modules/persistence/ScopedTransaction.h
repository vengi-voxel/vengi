/**
 * @file
 */

namespace persistence {

class Model;

class ScopedTransaction {
private:
	bool _commited = false;
	bool _autocommit;
	Model* _model;
public:
	ScopedTransaction(Model* model, bool autocommit = true);
	~ScopedTransaction();

	void commit();
	void rollback();
};

}
