#pragma once

#include <string>
#include <postgresql/libpq-fe.h>

namespace dbpost {

class PQConnect {
private:
	PGconn* _pgConnection;
	std::string _host;
	std::string _port;
	std::string _dbname;
	std::string _user;
	std::string _password;
public:
	PQConnect();
	~PQConnect();
	void setLoginData(const std::string& username, const std::string& password);
	void changeHost(const std::string& host);
	void changePort(const std::string& port);
	void changeDb(const std::string& dbname);
	void disconnect();
	int connect();
	PGconn* connection() const;
};

inline PGconn* PQConnect::connection() const {
	return _pgConnection;
}

}
