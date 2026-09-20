/**
 * Shared input-file filtering for emscripten convert tool UIs (palconvert, voxconvert).
 * Embedded into shell HTML via cmake CONVERT_FILE_FILTER_JS placeholder.
 */
var VengiConvertFileFilter = (function () {
	function fileExtension(name) {
		var i = String(name || '').lastIndexOf('.');
		return i >= 0 ? String(name).slice(i + 1).toLowerCase() : '';
	}

	function collectExtensions(formatGroups) {
		var inputExts = [];
		(formatGroups || []).forEach(function (group) {
			(group || []).forEach(function (item) {
				if (!item || item.noload) {
					return;
				}
				(item.extensions || []).forEach(function (ext) {
					ext = String(ext || '').toLowerCase();
					if (ext && inputExts.indexOf(ext) < 0) {
						inputExts.push(ext);
					}
				});
			});
		});
		inputExts.sort();
		return inputExts;
	}

	function applyInputAcceptFilter(formatGroups, options) {
		options = options || {};
		var inputExts = collectExtensions(formatGroups);
		if (typeof window !== 'undefined') {
			window.supportedInputExtensions = inputExts;
		}

		var accept = inputExts.map(function (e) {
			return '.' + e;
		}).join(',');
		var fileInput = document.getElementById(options.fileInputId || 'fileInput');
		if (fileInput) {
			fileInput.accept = accept;
		}

		var hint = document.getElementById(options.hintId || 'dropzoneHint');
		if (hint) {
			var base = options.hintBase || 'Drag & drop files here or click to choose';
			hint.textContent = accept ? (base + ' (' + accept.replace(/,/g, ', ') + ')') : base;
		}
		return inputExts;
	}

	function isAcceptedInputFile(file, extensions) {
		var exts = extensions;
		if (!exts && typeof window !== 'undefined') {
			exts = window.supportedInputExtensions;
		}
		if (!exts || !exts.length) {
			return false;
		}
		return exts.indexOf(fileExtension(file && file.name)) >= 0;
	}

	/**
	 * Append accepted files to selectedFiles (mutates). Logs rejected names via options.log.
	 */
	function mergeAcceptedFiles(fileList, selectedFiles, options) {
		options = options || {};
		var logFn = options.log || function () {};
		var rejected = [];
		for (var i = 0; i < fileList.length; i++) {
			var f = fileList[i];
			if (!isAcceptedInputFile(f, options.extensions)) {
				rejected.push(f.name);
				continue;
			}
			var exists = false;
			for (var j = 0; j < selectedFiles.length; j++) {
				var s = selectedFiles[j];
				if (s.name === f.name && s.size === f.size && s.lastModified === f.lastModified) {
					exists = true;
					break;
				}
			}
			if (!exists) {
				selectedFiles.push(f);
			}
		}
		if (rejected.length) {
			var exts = (options.extensions || (typeof window !== 'undefined' ? window.supportedInputExtensions : null) || [])
				.map(function (e) {
					return '.' + e;
				})
				.join(', ');
			logFn('Skipped unsupported input file' + (rejected.length > 1 ? 's' : '') + ': ' + rejected.join(', ') +
				(exts ? ' (accepted: ' + exts + ')' : ''));
		}
		return rejected;
	}

	/**
	 * Collect format groups from get_supported_formats_json() payload.
	 * Uses known keys; any other array-of-formats values are included too.
	 */
	function formatGroupsFromJson(json) {
		var groups = [];
		var known = ['voxels', 'images', 'palettes'];
		var seen = {};
		known.forEach(function (key) {
			if (json && Array.isArray(json[key])) {
				groups.push(json[key]);
				seen[key] = true;
			}
		});
		if (json) {
			Object.keys(json).forEach(function (key) {
				if (seen[key]) {
					return;
				}
				var val = json[key];
				if (Array.isArray(val) && val.length && val[0] && Array.isArray(val[0].extensions)) {
					groups.push(val);
				}
			});
		}
		return groups;
	}

	return {
		fileExtension: fileExtension,
		collectExtensions: collectExtensions,
		applyInputAcceptFilter: applyInputAcceptFilter,
		isAcceptedInputFile: isAcceptedInputFile,
		mergeAcceptedFiles: mergeAcceptedFiles,
		formatGroupsFromJson: formatGroupsFromJson
	};
})();
