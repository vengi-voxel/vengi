#pragma once

#include <unordered_map>
#include <postgresql/libpq-fe.h>
#include "core/NonCopyable.h"
#include "PQConnect.h"
#include "StoreInterface.h"

typedef std::unordered_map<std::string, std::string> TStrStrMap;
typedef std::pair<std::string, std::string> TStrStrPair;

namespace dbpost {

class PQStore  : public core::NonCopyable {
private:
	std::string sqlBuilder(const StoreInterface& model, bool update) const;
	std::string sqlLoadBuilder(const StoreInterface& model, bool update) const;
	PQConnect* _connection;
	bool _usable;
	PGresult* _res;
	std::string _lastErrorMsg;
	ExecStatusType _lastState;
	int _affectedRows;
public:
	PQStore(PQConnect* conn);
	bool query(const std::string& query);
	void trBegin();
	void trEnd();
	bool checkLastResult();
	void storeModel(StoreInterface& model);
	void createNeeds(const StoreInterface& model);
	std::unordered_map<std::string, std::string> loadModel(const StoreInterface& model);
	~PQStore();

	PGresult* result() const;
};

inline PGresult* PQStore::result() const {
	return _res;
}
}
