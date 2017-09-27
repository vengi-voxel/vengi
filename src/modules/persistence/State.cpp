#include "State.h"

namespace persistence {

State::State(ResultType* _res) :
		res(_res) {
}

State::State(State&& other) :
		res(other.res), lastErrorMsg(other.lastErrorMsg), affectedRows(
				other.affectedRows), result(other.result) {
	other.res = nullptr;
}

State::~State() {
	if (res != nullptr) {
		PQclear(res);
		res = nullptr;
	}
}

}
