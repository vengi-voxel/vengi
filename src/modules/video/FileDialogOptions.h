/**
 * @file
 */

#pragma once

#include "OpenFileMode.h"
#include "core/String.h"
#include "core/Common.h"
#include "io/FilesystemEntry.h"
#include "io/FormatDescription.h"

#include "core/Function.h"

namespace io {
struct FormatDescription;
}

namespace video {

using FileDialogSelectionCallback = core::Function<void(const core::String &, const io::FormatDescription *desc)>;
using FileDialogOptionsCallback =
	core::Function<bool(video::OpenFileMode mode, const io::FormatDescription *desc, const io::FilesystemEntry &entry)>;
using FileDialogPreviewCallback = core::Function<void(const io::FilesystemEntry &entry, video::OpenFileMode mode,
													  const io::FormatDescription *desc)>;

/**
 * @brief Options popup callback plus optional side-panel preview for the file dialog.
 *
 * Constructible from any options-only callable (lambda, functor) for backwards compatibility.
 */
struct FileDialogOptions {
	FileDialogOptionsCallback options;
	FileDialogPreviewCallback preview;

	FileDialogOptions() = default;
	FileDialogOptions(decltype(nullptr)) {
	}
	FileDialogOptions(const FileDialogOptions &) = default;
	FileDialogOptions(FileDialogOptions &&) = default;
	FileDialogOptions &operator=(const FileDialogOptions &) = default;
	FileDialogOptions &operator=(FileDialogOptions &&) = default;

	template <typename F,
			  typename = typename core::enable_if<
				  !core::is_same<typename core::decay<F>::type, FileDialogOptions>::value>::type>
	FileDialogOptions(F &&f) : options(core::forward<F>(f)) { // NOLINT(bugprone-forwarding-reference-overload)
	}

	explicit operator bool() const {
		return (bool)options;
	}

	bool operator()(video::OpenFileMode mode, const io::FormatDescription *desc,
					const io::FilesystemEntry &entry) const {
		if (!options) {
			return false;
		}
		return options(mode, desc, entry);
	}
};

} // namespace video
