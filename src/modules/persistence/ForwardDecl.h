/**
 * @file
 */

#pragma once

struct pg_conn;
struct pg_result;

typedef struct pg_conn PGconn;
typedef struct pg_result PGresult;

namespace persistence {

using ConnectionType = ::PGconn;
using ResultType = ::PGresult;

class Connection;
class DBHandler;
class Model;

}
