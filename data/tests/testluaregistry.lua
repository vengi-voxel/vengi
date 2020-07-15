
local function printtable(t)
	a = {}
	for n in pairs(t) do
		table.insert(a, n)
	end
	table.sort(a)
	for i,n in ipairs(a) do
		print(n, t[n])
	end
end

--[[
LuaTest is our BFG - call every possible ai, chr, zone, group and aggromgr function in here and validate it.
--]]
local luatest = REGISTRY.createNode("LuaTest")
function luatest:execute(ai, deltaMillis)
	--print("LuaTest node execute called with parameters: ai=["..tostring(ai).."], deltaMillis=["..tostring(deltaMillis).."]")
	local chr = ai:character()
	if chr == nil then
		print("error: ai has no character assigned")
		return FAILED
	end
	local pos = chr:position()
	if pos == nil then
		print("error: could not get character position")
		return FAILED
	end
	local x = pos.x
	pos.x = pos.x + 1.0
	pos.y = 0.5
	pos.z = 1.5
	if chr:position().x ~= x then
		print("error: modifying a vec should not update the character position")
		return FAILED
	end
	if pos.r ~= x + 1.0 then
		print("error: pos.r/x should be x+1.0")
		return FAILED
	end
	local zone = ai:zone()
	if zone == nil then
		print("error: ai has no zone assigned")
		return FAILED
	end
	local groupMgr = zone:groupMgr()
	local state = groupMgr:add(1, ai)
	if not state then
		print("error: could not add ai to group 1")
		return FAILED
	end
	if not groupMgr:isLeader(1, ai) then
		print("error: ai should lead group 1")
		return FAILED
	end
	local leader = groupMgr:leader(1)
	if leader ~= ai then
		print("error: ai should lead group 1")
		return FAILED
	end
	if not groupMgr:isInGroup(1, ai) then
		print("error: ai should be in group 1")
		return FAILED
	end
	if not groupMgr:isInAnyGroup(ai) then
		print("error: ai should be in a group")
		return FAILED
	end
	local stateRemove = groupMgr:remove(1, ai)
	if not stateRemove then
		print("error: could not remove ai to group 1")
		return FAILED
	end
	if groupMgr:isLeader(1, ai) then
		print("error: ai should not lead group 1")
		return FAILED
	end
	if groupMgr:isInAnyGroup(ai) then
		print("error: ai should not be in a group")
		return FAILED
	end
	-- we can't do anything with this, as the position is only updated with the next tick.
	groupMgr:position(1);
	local aiFromZone = zone:ai(chr:id())
	if aiFromZone == nil then
		print("error: could not get ai from zone with id " .. chr:id())
		return FAILED
	end
	local aggroMgr = ai:aggroMgr()
	aggroMgr:addAggro(3, 0.3)
	aggroMgr:addAggro(4, 0.4)
	aggroMgr:addAggro(5, 0.5)
	aggroMgr:setReduceByRatio(0.5, 2.0)
	aggroMgr:setReduceByValue(1.0)
	aggroMgr:resetReduceValue()
	local aggroVal = aggroMgr:addAggro(2, 1.0)
	if aggroVal ~= 1.0 then
		print("error: expected aggroVal was 1.0 - but found was " .. aggroVal)
		return FAILED
	end
	local entries = aggroMgr:entries()
	id, val = aggroMgr:highestEntry()
	if id ~= 2 then
		print("error: expected id was 2 - but found was " .. id)
		return FAILED
	end
	if val ~= aggroVal then
		print("error: expected value was " .. aggroVal .. " - but found was " .. val)
		return FAILED
	end
	chr:setAttribute("Key", "Value")
	chr:setAttribute("Key2", "Value2")
	chr:setAttribute("Attribute", "1.0")
	if chr:attributes()["Key"] ~= "Value" then
		print("error: expected attribute for 'Key' is 'Value' - but found was " .. chr:attributes()["Key"])
		return FAILED
	end
	local execute = function (ai)
		--print("zone execution for ai: " .. ai:id());
	end
	zone:execute(execute)
	local filteredEntities = ai:filteredEntities()
	local newFilteredEntities = {2, 3, 4, 5}
	ai:setFilteredEntities(newFilteredEntities)
	filteredEntities = ai:filteredEntities()
	if filteredEntities[1] ~= 2 then
		print("error: unexpected value at index 1" .. filteredEntities[1])
		return FAILED
	end
	if filteredEntities[2] ~= 3 then
		print("error: unexpected value at index 2" .. filteredEntities[2])
		return FAILED
	end
	if filteredEntities[3] ~= 4 then
		print("error: unexpected value at index 3" .. filteredEntities[3])
		return FAILED
	end
	if filteredEntities[4] ~= 5 then
		print("error: unexpected value at index 4" .. filteredEntities[4])
		return FAILED
	end
	-- doesn't belong here, but into the filters...
	--[[
	print("id (ai): " .. ai:id())
	print("id (chr - should be the same as ai): " .. chr:id())
	print("filtered entities:")
	printtable(filteredEntities)
	print("time: " .. ai:time())
	print("haszone: " .. tostring(ai:hasZone()))
	if ai:hasZone() then
		print("zone: " .. tostring(ai:zone()))
	end
	print("aggroentries:")
	printtable(entries)
	print("attributes:")
	printtable(chr:attributes())
	print("position: " .. tostring(pos))
	print("position x: " .. pos.x)
	print("position y: " .. pos.y)
	print("position z: " .. pos.z)
	print("character: " .. tostring(chr))
	print("aggromgr: " .. tostring(aggroMgr))
	--]]
	return FINISHED
end

--[[
LuaTest2 will just return a different tree node state
--]]
local luatest2 = REGISTRY.createNode("LuaTest2")
function luatest2:execute(ai, deltaMillis)
	--print("LuaTest2 node execute called with parameters: ai=["..tostring(ai).."], deltaMillis=["..tostring(deltaMillis).."]")
	return RUNNING
end

--[[
ensure we have a name clash here with a node
--]]
local luaconditiontest = REGISTRY.createCondition("LuaTest")
function luaconditiontest:evaluate(ai)
	--print("LuaTest condition evaluate called with parameter: ai=["..tostring(ai).."]")
	return true
end

--[[
A condition that always returns true
--]]
local luaconditiontesttrue = REGISTRY.createCondition("LuaTestTrue")
function luaconditiontesttrue:evaluate(ai)
	return ai:id() <= 9000
end

--[[
A condition that always returns false
--]]
local luaconditiontestfalse = REGISTRY.createCondition("LuaTestFalse")
function luaconditiontestfalse:evaluate(ai)
	return ai:id() > 9000
end

--[[
A filter to test the filter creation
--]]
local luafiltertest = REGISTRY.createFilter("LuaFilterTest")
function luafiltertest:filter(ai)
	ai:addFilteredEntity(42)
	ai:addFilteredEntity(1337)
	ai:addFilteredEntity(101)
	local ents = ai:filteredEntities()
	ai:setFilteredEntities(ents)
end

--[[
An empty steering node to test the filter creation
--]]
local luasteeringtest = REGISTRY.createSteering("LuaSteeringTest")
function luasteeringtest:execute(ai, speed)
	return 0.0, 1.0, 0.0, 0.6
end

