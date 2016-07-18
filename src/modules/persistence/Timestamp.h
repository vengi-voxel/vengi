#pragma once

namespace persistence {

class Timestamp {
private:
	uint64_t _time;
	bool _now;
public:
	// TODO: POSTGRES: microseconds, sqlite seconds, mysql seconds
	Timestamp(uint64_t time) :
			_time(time), _now(false) {
	}

	Timestamp() :
		Timestamp(0L) {
	}

	static Timestamp now() {
		static Timestamp nowInstance;
		nowInstance._now = true;
		return nowInstance;
	}

	inline bool isNow() const {
		return _now;
	}

	uint64_t time() const {
		return _time;
	}
};

}
