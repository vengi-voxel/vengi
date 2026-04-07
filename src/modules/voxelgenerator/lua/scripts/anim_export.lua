--
-- Export animation data from the current scene to a JSON file.
--
-- This script iterates all nodes and all animations, collecting keyframe
-- data (frame index, interpolation type, local translation, local
-- orientation, local scale). The result is written as a JSON file that
-- can later be imported into another model with matching node names
-- using the anim_import.lua script.
--
-- Node matching between models is done by **name** so the target model
-- must use the same (or similar) node names as the source.
--

local JSON = require "modules.JSON"

function arguments()
	return {
		{ name = "filename", desc = "Output JSON file name", type = "str", default = "animation.json" },
		{ name = "animation", desc = "Animation to export (empty = all)", type = "str", default = "" },
	}
end

function description()
	return "Export animation keyframe data to a JSON file. "
		.. "The file can be imported into another rigged model with matching node names "
		.. "using the anim_import script."
end

function main(_, _, _, filename, animFilter)
	local allAnims = g_scenegraph.animations()
	local activeAnim = g_scenegraph.activeAnimation()

	-- Determine which animations to export
	local animsToExport = {}
	if animFilter ~= nil and animFilter ~= "" then
		if not g_scenegraph.hasAnimation(animFilter) then
			error("Animation '" .. animFilter .. "' not found")
		end
		animsToExport[#animsToExport + 1] = animFilter
	else
		for _, a in ipairs(allAnims) do
			animsToExport[#animsToExport + 1] = a
		end
	end

	local allNodeIds = g_scenegraph.nodeIds()

	-- Build the export structure
	local exportData = {
		version = 1,
		animations = {},
	}

	for _, animName in ipairs(animsToExport) do
		g_scenegraph.setAnimation(animName)

		local animEntry = {
			name = animName,
			nodes = {},
		}

		for _, nodeId in ipairs(allNodeIds) do
			local node = g_scenegraph.get(nodeId)
			local numKfs = node:numKeyFrames()
			if numKfs > 0 then
				local nodeEntry = {
					name = node:name(),
					keyframes = {},
				}

				for i = 0, numKfs - 1 do
					local kf = node:keyFrame(i)
					local lt = kf:localTranslation()
					local lo = kf:localOrientation()
					local ls = kf:localScale()

					nodeEntry.keyframes[#nodeEntry.keyframes + 1] = {
						frame = kf:frame(),
						interpolation = kf:interpolation(),
						translation = { x = lt.x, y = lt.y, z = lt.z },
						orientation = { x = lo.x, y = lo.y, z = lo.z, w = lo.w },
						scale = { x = ls.x, y = ls.y, z = ls.z },
					}
				end

				animEntry.nodes[#animEntry.nodes + 1] = nodeEntry
			end
		end

		exportData.animations[#exportData.animations + 1] = animEntry
	end

	-- Restore the originally active animation
	g_scenegraph.setAnimation(activeAnim)

	-- Write JSON to file
	local jsonStr = JSON:encode(exportData)
	local stream = g_io.open(filename, "w")
	stream:writeString(jsonStr, false)
	stream:close()

	local count = 0
	for _, a in ipairs(exportData.animations) do
		count = count + #a.nodes
	end
	g_log.info("Exported " .. #exportData.animations .. " animation(s) with "
		.. count .. " node track(s) to " .. filename)
end
