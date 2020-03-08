local luanode = REGISTRY.createNode("Test")
function luanode:execute(ai, deltaMillis)
  print("Node execute called with parameters: ai=["..tostring(ai).."], deltaMillis=["..tostring(deltaMillis).."]")
  return FINISHED
end
