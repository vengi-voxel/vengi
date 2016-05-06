/**
 * @file
 */

#pragma once

#include <string>
#include <libpq-fe.h>

namespace persistence {

class Connection {
private:
	PGconn* _pgConnection;
	std::string _host;
	std::string _dbname;
	std::string _user;
	std::string _password;
	uint16_t _port;

	std::string escape(const std::string& value) const;
public:
	Connection();

	~Connection();

	void setLoginData(const std::string& username, const std::string& password);

	void changeHost(const std::string& host);

	void changePort(uint16_t port);

	void changeDb(const std::string& dbname);

	void disconnect();

	bool connect();

	PGconn* connection() const;
};

inline PGconn* Connection::connection() const {
	return _pgConnection;
}

}
