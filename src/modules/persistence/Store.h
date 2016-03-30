#pragma once

#include "Connection.h"
#include "PeristenceModel.h"
#include <unordered_map>
#include <postgresql/libpq-fe.h>
#include "core/NonCopyable.h"

namespace persistence {

typedef std::unordered_map<std::string, std::string> KeyValueMap;
typedef std::pair<std::string, std::string> KeyValuePair;

class Store : public core::NonCopyable {
private:
	Connection* _connection;
	bool _usable;
	PGresult* _res;
	std::string _lastErrorMsg;
	ExecStatusType _lastState;
	int _affectedRows;

	std::string sqlBuilder(const PeristenceModel& model, bool update) const;

	std::string sqlLoadBuilder(const PeristenceModel& model, bool update) const;
public:
	Store(Connection* conn);
	~Store();

	bool query(const std::string& query);

	void trBegin();

	void trEnd();

	bool checkLastResult();

	bool storeModel(PeristenceModel& model);

	bool createNeeds(const PeristenceModel& model);

	KeyValueMap loadModel(const PeristenceModel& model);

	PGresult* result() const;
};

inline PGresult* Store::result() const {
	return _res;
}

}
