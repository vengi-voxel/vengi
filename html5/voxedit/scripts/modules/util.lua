local module = {}

function module.getFileExtension(path)
	return path:match("^.+(%..+)$")
end

function module.getFileFromPath(path)
	return path:match("^.+/(.+)$")
end

return module
