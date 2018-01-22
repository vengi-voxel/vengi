/**
 * @file
 */

#pragma once

#include <memory>

struct pg_conn;
struct pg_result;

typedef struct pg_conn PGconn;
typedef struct pg_result PGresult;

namespace persistence {

using ConnectionType = ::PGconn;
using ResultType = ::PGresult;

class Connection;
class Model;

class DBHandler;
typedef std::shared_ptr<DBHandler> DBHandlerPtr;

class PersistenceMgr;
typedef std::shared_ptr<PersistenceMgr> PersistenceMgrPtr;

}
