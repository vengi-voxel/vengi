-- https://www.meshy.ai/
--
-- This script uses the Meshy API to generate a 3D model from a text prompt - it is not using the text-to-voxel
-- api as the results are not as good. The script will wait for the model to be generated and then import it into
-- the scene. The script requires a meshy api key to be set as a cvar called meshy_api_key.
-- We are using the glb/gltf import for this.
--
-- https://docs.meshy.ai/api-text-to-3d-beta
--
-- To call this script you should export your api key as env var or set it from the command line or console
-- See the configuration section in the documentation.
-- To use the api key with an env variable you should add MESHY_API_KEY=XXX to your environment variables.

local JSON = require 'modules.JSON'
local url = require 'modules.url'
local util = require 'modules.util'

function arguments()
	return {
		{name = 'prompt', desc = '', type = 'string', default = 'A cute cat'},
		{name = 'negative_prompt', desc = '', type = 'string', default = ''},
		{name = 'scale', desc = '', type = 'float', default = '10.0'},
		{name = 'art_style', desc = '', type = 'enum', enum = 'realistic,sculpture,pbr', default = 'realistic'},
		{name = "api_key", desc = "The Meshy API key (cvar meshy_api_key)", type = "string", default = ""}
	}
end

function downloadFiles(retrievalJson, headers)
	local modelUrl = retrievalJson.model_urls.glb
	local textures = retrievalJson.texture_urls
	if textures ~= nil then
		g_log.info("Textures: " .. #textures)
		for _, v in pairs(textures) do
			local baseColorUrl = url.parse(v.base_color)
			local baseColorFile = util.getFileFromPath(baseColorUrl.path)
			local texture = g_io.open(baseColorFile, 'w')
			local textureStream, _ = g_http.get(v.base_color, headers)
			texture:writeStream(textureStream)
			texture:close()
			g_log.info("Downloaded texture: " .. baseColorFile)
		end
	else
		g_log.info("No textures found")
	end
	local modelStream, _ = g_http.get(modelUrl, headers)
	local glbFile = retrievalJson.id .. ".glb"
	local file = g_io.open(glbFile, 'w')
	file:writeStream(modelStream)
	file:close()
	g_log.info("Downloaded glb: " .. glbFile)
end

function main(_, _, _, prompt, negative_prompt, scale, art_style, api_key)
	local body =
		JSON:encode({mode = 'preview', prompt = prompt, art_style = art_style, negative_prompt = negative_prompt})
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
	local stream, _ = g_http.post('https://api.meshy.ai/v2/text-to-3d', body, headers)
	local str = stream:readString()
	local json = JSON:decode(str)
	if json == nil then
		error('Failed to parse JSON response: ' .. str)
	end
	local resultId = json.result
	print('resultId: ' .. resultId)

	while true do
		local retrieval = g_http.get('https://api.meshy.ai/v2/text-to-3d/' .. resultId, headers)
		local retrievalStr = retrieval:readString()
		local retrievalJson = JSON:decode(retrievalStr)
		if retrievalJson == nil then
			error('Failed to parse JSON retrieval response: ' .. retrievalStr)
		end
		if retrievalJson.status == 'PENDING' or retrievalJson.status == 'IN_PROGRESS' then
			g_log.info("Model is not yet ready, waiting with status " .. retrievalJson.status .. " progress: " .. retrievalJson.progress)
			g_sys.sleep(4000)
			coroutine.yield();
		elseif retrievalJson.status == 'SUCCEEDED' then
			downloadFiles(retrievalJson, headers)
			local oldVal = g_var.float('voxformat_scale')
			g_var.setFloat('voxformat_scale', scale)
			g_import.scene(retrievalJson.id .. ".glb")
			g_var.setFloat('voxformat_scale', oldVal)
			break
		else
			error('Failed to generate 3D model: ' .. retrievalJson.status)
		end
	end
end
