/**
 * @file
 */

#pragma once

#include "engine-config.h"
#ifdef HAVE_POSTGRES
#include <libpq-fe.h>
#endif

namespace persistence {

extern bool postgresInit();
extern void postgresShutdown();

#ifdef HAVE_POSTGRES
#define DYNDEFINE(NAME) decltype(::NAME) *NAME

DYNDEFINE(PQescapeStringConn);
DYNDEFINE(PQescapeString);
DYNDEFINE(PQescapeByteaConn);
DYNDEFINE(PQescapeBytea);
DYNDEFINE(PQfreemem);
DYNDEFINE(PQexec);
DYNDEFINE(PQprepare);
DYNDEFINE(PQexecPrepared);
DYNDEFINE(PQexecParams);
DYNDEFINE(PQresultStatus);
DYNDEFINE(PQresStatus);
DYNDEFINE(PQresultErrorMessage);
DYNDEFINE(PQresultErrorField);
DYNDEFINE(PQcmdTuples);
DYNDEFINE(PQcmdStatus);
DYNDEFINE(PQgetvalue);
DYNDEFINE(PQgetlength);
DYNDEFINE(PQgetisnull);
DYNDEFINE(PQoidValue);
DYNDEFINE(PQoidStatus);
DYNDEFINE(PQfformat);
DYNDEFINE(PQntuples);
DYNDEFINE(PQnfields);
DYNDEFINE(PQnparams);
DYNDEFINE(PQclear);
DYNDEFINE(PQfinish);
DYNDEFINE(PQstatus);
DYNDEFINE(PQconnectdb);
DYNDEFINE(PQerrorMessage);
DYNDEFINE(PQinitSSL);
DYNDEFINE(PQsetdbLogin);
DYNDEFINE(PQsslInUse);
DYNDEFINE(PQsetNoticeProcessor);
DYNDEFINE(PQflush);
DYNDEFINE(PQfname);
#undef DYNDEFINE
#endif
}
