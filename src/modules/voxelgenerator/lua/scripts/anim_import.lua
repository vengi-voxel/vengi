--
-- Import animation data from a JSON file and apply it to the current scene.
--
-- The JSON file is produced by the anim_export script. Nodes are matched
-- by name so the target model must use the same (or similar) node names
-- as the source model that the animation was exported from.
--
-- This is the counterpart of anim_export.lua and together they allow
-- transferring animations between different rigged models, similar to
-- how VoxEdit's decompile tool transfers animations by matching node IDs.
--

local JSON = require "modules.JSON"

function arguments()
	return {
		{ name = "filename", desc = "Input JSON file name", type = "file", default = "animation.json" },
		{ name = "animation", desc = "Animation to import (empty = all)", type = "str", default = "" },
		{ name = "replace", desc = "Replace existing animations with the same name", type = "bool", default = "true" },
	}
end

function description()
	return "Import animation keyframe data from a JSON file exported by anim_export. "
		.. "Nodes are matched by name so the target model should have matching node names."
end

-- Remove all keyframes except the first one for a given node
local function clearKeyFrames(node)
	local count = node:numKeyFrames()
	for i = count - 1, 1, -1 do
		node:removeKeyFrame(i)
	end
end

function main(_, _, _, filename, animFilter, replace)
	local stream = g_io.open(filename, "r")
	local jsonStr = stream:readString()
	stream:close()

	local data = JSON:decode(jsonStr)
	if data == nil then
		error("Failed to parse JSON from " .. filename)
	end
	if data.version ~= 1 then
		error("Unsupported animation file version: " .. tostring(data.version))
	end

	-- Build a lookup table: node name -> node object
	local allNodeIds = g_scenegraph.nodeIds()
	local nodesByName = {}
	for _, nodeId in ipairs(allNodeIds) do
		local node = g_scenegraph.get(nodeId)
		local name = node:name()
		-- If multiple nodes share the same name, keep the first one found
		if nodesByName[name] == nil then
			nodesByName[name] = node
		end
	end

	local activeAnim = g_scenegraph.activeAnimation()
	local importedCount = 0
	local skippedNodes = 0

	for _, animEntry in ipairs(data.animations) do
		local animName = animEntry.name

		-- Apply animation filter
		if animFilter ~= nil and animFilter ~= "" and animName ~= animFilter then
			goto continue_anim
		end

		-- Handle existing animation
		if g_scenegraph.hasAnimation(animName) then
			if not replace then
				g_log.warn("Animation '" .. animName .. "' already exists, skipping (replace=false)")
				goto continue_anim
			end
			-- Switch to it and clear keyframes for all matching nodes
			g_scenegraph.setAnimation(animName)
			for _, nodeEntry in ipairs(animEntry.nodes) do
				local targetNode = nodesByName[nodeEntry.name]
				if targetNode ~= nil then
					clearKeyFrames(targetNode)
				end
			end
		else
			g_scenegraph.addAnimation(animName)
			g_scenegraph.setAnimation(animName)
		end

		-- Import keyframes for each node
		for _, nodeEntry in ipairs(animEntry.nodes) do
			local targetNode = nodesByName[nodeEntry.name]
			if targetNode == nil then
				g_log.debug("Node '" .. nodeEntry.name .. "' not found in target model, skipping")
				skippedNodes = skippedNodes + 1
				goto continue_node
			end

			-- Apply keyframes
			for kfIdx, kfData in ipairs(nodeEntry.keyframes) do
				local frame = kfData.frame
				local kf
				if kfIdx == 1 and targetNode:hasKeyFrameForFrame(frame) then
					-- Reuse the existing first keyframe (frame 0 default)
					kf = targetNode:keyFrameForFrame(frame)
				elseif targetNode:hasKeyFrameForFrame(frame) then
					kf = targetNode:keyFrameForFrame(frame)
				else
					kf = targetNode:addKeyFrame(frame)
				end

				-- Set interpolation type
				if kfData.interpolation ~= nil then
					kf:setInterpolation(kfData.interpolation)
				end

				-- Set local translation
				local t = kfData.translation
				if t ~= nil then
					kf:setLocalTranslation(t.x, t.y, t.z)
				end

				-- Set local orientation
				local o = kfData.orientation
				if o ~= nil then
					kf:setLocalOrientation(o.x, o.y, o.z, o.w)
				end

				-- Set local scale
				local sc = kfData.scale
				if sc ~= nil then
					kf:setLocalScale(sc.x, sc.y, sc.z)
				end
			end

			::continue_node::
		end

		importedCount = importedCount + 1
		::continue_anim::
	end

	-- Restore the originally active animation or switch to first imported
	if g_scenegraph.hasAnimation(activeAnim) then
		g_scenegraph.setAnimation(activeAnim)
	end

	g_scenegraph.updateTransforms()

	g_log.info("Imported " .. importedCount .. " animation(s) from " .. filename)
	if skippedNodes > 0 then
		g_log.warn("Skipped " .. skippedNodes .. " node(s) not found in target model")
	end
end
