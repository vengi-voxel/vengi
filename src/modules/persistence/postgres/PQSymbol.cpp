/**
 * @file
 */

#define PQSYMBOL_IMPLEMENTATION
#include "PQSymbol.h"
#undef PQSYMBOL_IMPLEMENTATION
#include "core/Var.h"
#include "core/Log.h"
#include "core/GameConfig.h"
#include <SDL.h>

namespace persistence {

static void* obj = NULL;

#define DYNLOAD(HNDL, NAME) NAME = reinterpret_cast<decltype(NAME)>(SDL_LoadFunction(HNDL, #NAME))

void postgresShutdown() {
	SDL_UnloadObject(obj);

#ifdef HAVE_POSTGRES
	PQescapeStringConn = nullptr;
	PQescapeString = nullptr;
	PQescapeByteaConn = nullptr;
	PQescapeBytea = nullptr;
	PQfreemem = nullptr;
	PQexec = nullptr;
	PQprepare = nullptr;
	PQexecPrepared = nullptr;
	PQexecParams = nullptr;
	PQresStatus = nullptr;
	PQresultStatus = nullptr;
	PQresultErrorMessage = nullptr;
	PQresultErrorField = nullptr;
	PQcmdStatus = nullptr;
	PQcmdTuples = nullptr;
	PQgetvalue = nullptr;
	PQgetlength = nullptr;
	PQgetisnull = nullptr;
	PQoidStatus = nullptr;
	PQoidValue = nullptr;
	PQfformat = nullptr;
	PQntuples = nullptr;
	PQnfields = nullptr;
	PQnparams = nullptr;
	PQclear = nullptr;
	PQfinish = nullptr;
	PQconnectdb = nullptr;
	PQstatus = nullptr;
	PQerrorMessage = nullptr;
	PQinitSSL = nullptr;
	PQsetdbLogin = nullptr;
	PQsslInUse = nullptr;
	PQsetNoticeProcessor = nullptr;
	PQflush = nullptr;
	PQfname = nullptr;
#endif
}

bool postgresInit() {
#ifdef HAVE_POSTGRES
	const core::VarPtr& path = core::Var::get(cfg::ServerPostgresLib);
	if (path) {
		obj = SDL_LoadObject(path->strVal().c_str());
	}

	if (obj == nullptr) {
		const char *searchPaths[] = {"libpq.so", nullptr};
		for (const char **searchPath = searchPaths; *searchPath; ++searchPath) {
			obj = SDL_LoadObject(*searchPath);
			if (obj != nullptr) {
				break;
			}
		}
	}

	if (obj == nullptr) {
		Log::error("Could not load libpg");
		return false;
	}

	DYNLOAD(obj, PQescapeStringConn);
	DYNLOAD(obj, PQescapeString);
	DYNLOAD(obj, PQescapeByteaConn);
	DYNLOAD(obj, PQescapeBytea);
	DYNLOAD(obj, PQfreemem);
	DYNLOAD(obj, PQexec);
	DYNLOAD(obj, PQprepare);
	DYNLOAD(obj, PQexecPrepared);
	DYNLOAD(obj, PQexecParams);
	DYNLOAD(obj, PQresStatus);
	DYNLOAD(obj, PQresultStatus);
	DYNLOAD(obj, PQresultErrorMessage);
	DYNLOAD(obj, PQresultErrorField);
	DYNLOAD(obj, PQcmdStatus);
	DYNLOAD(obj, PQcmdTuples);
	DYNLOAD(obj, PQgetvalue);
	DYNLOAD(obj, PQgetlength);
	DYNLOAD(obj, PQgetisnull);
	DYNLOAD(obj, PQoidStatus);
	DYNLOAD(obj, PQoidValue);
	DYNLOAD(obj, PQfformat);
	DYNLOAD(obj, PQntuples);
	DYNLOAD(obj, PQnfields);
	DYNLOAD(obj, PQnparams);
	DYNLOAD(obj, PQclear);
	DYNLOAD(obj, PQfinish);
	DYNLOAD(obj, PQconnectdb);
	DYNLOAD(obj, PQstatus);
	DYNLOAD(obj, PQerrorMessage);
	DYNLOAD(obj, PQinitSSL);
	DYNLOAD(obj, PQsetdbLogin);
	DYNLOAD(obj, PQsslInUse);
	DYNLOAD(obj, PQsetNoticeProcessor);
	DYNLOAD(obj, PQflush);
	DYNLOAD(obj, PQfname);

	if (PQescapeStringConn == nullptr || PQexec == nullptr
			|| PQinitSSL == nullptr || PQsetdbLogin == nullptr
			|| PQsslInUse == nullptr || PQsetNoticeProcessor == nullptr
			|| PQflush == nullptr || PQfname == nullptr) {
		Log::error("Could not load all the needed symbols from libpg");
		return false;
	}
	return true;
#else
	Log::error("No postgres support");
	return false;
#endif
}

#undef DYNLOAD

}
