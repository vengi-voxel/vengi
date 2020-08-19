local bones = require 'chr.bones'
local shared = require 'chr.shared'

function init()
  shared.setupMeshTypes()
  bones.setupBones()
  shared.setBasePath("undead", "male")
  shared.setAllPaths("zombie")
  settings.setPath("shoulder", "")

  local attributes = shared.defaultSkeletonAttributes()
  attributes["headHeight"] = 14.0
  attributes["chestHeight"] = 1.0
  attributes["invisibleLegHeight"] = 0.4
  attributes["footRight"] = -1.2
  return attributes
end
