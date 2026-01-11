/**
 * @file
 */

#pragma once

#include "core/IComponent.h"
#include "core/String.h"
#include "core/Var.h"

namespace io {
class WriteStream;
}

namespace app {

class Pipe : public core::IComponent {
private:
#if defined(_WIN32)
	void *_pipe = nullptr; // HANDLE
	void *_connectEvent = nullptr; // HANDLE
	bool _connected = false;
#else
	int _pipe = -1;
#endif
	core::String _pipeName;
	core::VarPtr _corePipe;

public:
	Pipe();
	virtual ~Pipe();

	void construct() override;
	bool init() override;
	void shutdown() override;

	/**
	 * @brief Read from the pipe into the stream
	 * @return read bytes
	 */
	int read(io::WriteStream &stream);
};

} // namespace app
