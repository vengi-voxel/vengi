local entities = require("shared.entities")

function init()
  entities.register("attrib")

  local player = attrib.createContainer("PLAYER")
  player:addAbsolute("SPEED", 20.0)
  player:addAbsolute("HEALTH", 100.0)
  player:addAbsolute("ATTACKRANGE", 1.0)
  player:addAbsolute("STRENGTH", 1.0)
  player:addAbsolute("VIEWDISTANCE", 500.0)
end
