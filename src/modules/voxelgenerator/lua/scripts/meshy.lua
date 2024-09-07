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

function arguments()
	return {
		{name = 'prompt', desc = '', type = 'string', default = 'A cute cat'},
		{name = 'negative_prompt', desc = '', type = 'string', default = ''},
		{name = 'art_style', desc = '', type = 'enum', enum = 'realistic,cartoon,low-poly,sculpture,pbr', default = 'low-poly'}
	}
end

function main(_, _, _, prompt, negative_prompt, art_style)
	local body =
		JSON:encode({mode = 'preview', prompt = prompt, art_style = art_style, negative_prompt = negative_prompt})
	g_var.create('meshy_api_key', '', 'meshy ai api key', false, true)
	local apiKey = g_var.str('meshy_api_key')
	if apiKey == nil or apiKey == '' then
		error('Please set the meshy_api_key variable')
	end

	local headers = {}
	headers['Authorization'] = 'Bearer ' .. apiKey
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
		elseif retrievalJson.status == 'SUCCEEDED' then
			local modelUrl = retrievalJson.model_urls.glb
			local modelStream, _ = g_http.get(modelUrl, headers)
			g_import.scene(retrievalJson.id .. ".glb", modelStream)
			break
		else
			error('Failed to generate 3D model: ' .. retrievalJson.status)
		end
	end
end
