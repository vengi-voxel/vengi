-- https://www.meshy.ai/
--
-- This script uses the Meshy API to generate a 3D model from a text prompt - it is not using the text-to-voxel
-- api as the results are not as good. The script will wait for the model to be generated and then import it into
-- the scene. The script requires a meshy api key to be set as a cvar called meshy_api_key.
-- We are using the glb/gltf import for this.
--
-- https://docs.meshy.ai/api-text-to-3d
--
-- To call this script you should export your api key as env var or set it from the command line or console
-- See the configuration section in the documentation.
-- To use the api key with an env variable you should add MESHY_API_KEY=XXX to your environment variables.

local JSON = require 'modules.JSON'
local url = require 'modules.url'
local util = require 'modules.util'

local c_baseurl = "https://api.meshy.ai/openapi/v2"

function arguments()
	return {
		{name = 'prompt', desc = '', type = 'string', default = 'A cute cat'},
		{name = 'negative_prompt', desc = '', type = 'string', default = ''},
		{name = 'texture_prompt', desc = 'Max. 600 chars', type = 'string', default = ''},
		{name = 'scale', desc = '', type = 'float', default = '10.0'},
		{name = 'art_style', desc = '', type = 'enum', enum = 'realistic,sculpture', default = 'realistic'},
		{name = "api_key", desc = "The Meshy API key (cvar meshy_api_key)", type = "string", default = ""}
	}
end

function description()
	return "Generates a 3D model from a text prompt using the Meshy API. The script will wait for the model to be generated and then import it into the scene."
end

function downloadFiles(headers, taskId)
	g_log.info('downloading results for task ' .. taskId)
	local retrieval = g_http.get(c_baseurl .. '/text-to-3d/' .. taskId, headers)
	local retrievalStr = retrieval:readString()
	g_log.debug("Retrieval: " .. retrievalStr)
	local retrievalJson = JSON:decode(retrievalStr)
	if retrievalJson ~= nil then
		local modelUrl = retrievalJson.model_urls.glb
		local textures = retrievalJson.texture_urls
		if textures ~= nil then
			g_log.info("Textures: " .. #textures)
			for _, v in pairs(textures) do
				local baseColorUrl = url.parse(v.base_color)
				g_log.info("Downloading texture: " .. baseColorUrl)
				local baseColorFile = util.getFileFromPath(baseColorUrl.path)
				local texture = g_io.open(baseColorFile, 'w')
				local textureStream, _ = g_http.get(v.base_color, headers)
				texture:writeStream(textureStream)
				texture:close()
			end
		else
			g_log.info("No textures found")
		end
		g_log.info("Downloading glb: " .. modelUrl)
		local modelStream, _ = g_http.get(modelUrl, headers)
		local glbFile = retrievalJson.id .. ".glb"
		local file = g_io.open(glbFile, 'w')
		file:writeStream(modelStream)
		file:close()
		return retrievalJson
	else
		error('Failed to parse JSON from response: ' .. retrievalStr)
	end
end

local function waitForTextTo3D(taskId, headers)
	g_log.info('wait for task ' .. taskId .. ' to finish')

	while true do
		local retrieval = g_http.get(c_baseurl .. '/text-to-3d/' .. taskId, headers)
		local retrievalStr = retrieval:readString()
		local retrievalJson = JSON:decode(retrievalStr)
		if retrievalJson ~= nil then
			if retrievalJson.status == 'PENDING' or retrievalJson.status == 'IN_PROGRESS' then
				g_log.info("Model is not yet ready, waiting with status " .. retrievalJson.status .. " progress: " .. retrievalJson.progress)
				g_sys.sleep(4000)
				coroutine.yield();
			elseif retrievalJson.status == 'SUCCEEDED' then
				break
			else
				local taskError = retrievalJson.task_error
				if taskError ~= nil then
					g_log.error("Task error: " .. taskError.message)
				end
				error('Failed to generate 3D model: ' .. retrievalJson.status)
			end
		else
			error('Failed to parse JSON from response: ' .. retrievalStr)
		end
	end
end

local function triggerTextTo3D(headers, prompt, negative_prompt, art_style)
	local body = JSON:encode(
		{
			mode = 'preview',
			prompt = prompt,
			art_style = art_style,
			enable_pbr = art_style == 'realistic',
			negative_prompt = negative_prompt,
			ai_model = 'meshy-5',
			topology = 'triangle'
		}
	)

	local stream, _ = g_http.post(c_baseurl .. '/text-to-3d', body, headers)
	local str = stream:readString()
	local json = JSON:decode(str)
	if json ~= nil then
		return json.result
	else
		error('Failed to parse JSON response: ' .. str)
	end
end

local function triggerRefineTask(headers, taskId, texture_prompt)
	local body = JSON:encode(
		{
			preview_task_id = taskId,
			mode = 'refine',
			texture_prompt = texture_prompt,
		}
	)

	local stream, _ = g_http.post(c_baseurl .. '/text-to-3d', body, headers)
	local str = stream:readString()
	local json = JSON:decode(str)
	if json ~= nil then
		return json.result
	else
		error('Failed to parse JSON response: ' .. str)
	end
end

function main(_, _, _, prompt, negative_prompt, texture_prompt, scale, art_style, api_key)
	g_var.create('meshy_api_key', '', 'meshy ai api key', false, true)
	local apiKeyVar = g_var.str('meshy_api_key')
	if apiKeyVar == nil or apiKeyVar == '' then
		if api_key ~= nil and api_key ~= '' then
			apiKeyVar = api_key
			g_var.setStr('meshy_api_key', apiKeyVar)
		else
			error('Please set the meshy_api_key variable')
		end
	end

	local headers = {}
	headers['Authorization'] = 'Bearer ' .. apiKeyVar
	headers['Content-Type'] = 'application/json'

	local taskIdPreview = triggerTextTo3D(headers, prompt, negative_prompt, art_style);
	waitForTextTo3D(taskIdPreview, headers)
	local taskIdRefinement = triggerRefineTask(headers, taskIdPreview, texture_prompt)
	waitForTextTo3D(taskIdRefinement, headers)
	local json = downloadFiles(headers, taskIdRefinement)
	local oldVal = g_var.float('voxformat_scale')
	g_var.setFloat('voxformat_scale', scale)
	g_import.scene(taskIdRefinement .. ".glb")
	g_var.setFloat('voxformat_scale', oldVal)
	if (json ~= nil) then
		local root = g_scenegraph.get(0)
		if (json.art_style ~= nil) then
			root:setProperty("meshy_art_style", json.art_style)
		end
		if (json.prompt ~= nil) then
			root:setProperty("meshy_prompt", json.prompt)
		end
		if (json.negative_prompt ~= nil) then
			root:setProperty("meshy_negative_prompt", json.negative_prompt)
		end
		if (json.seed ~= nil) then
			root:setProperty("meshy_seed", json.seed)
		end
		if (json.texture_prompt ~= nil) then
			root:setProperty("meshy_texture_prompt", json.texture_prompt)
		end
	end
end
