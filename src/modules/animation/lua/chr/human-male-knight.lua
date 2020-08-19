local bones = require 'chr.bones'
local shared = require 'chr.shared'

function init()
  shared.setupMeshTypes()
  bones.setupBones()
  shared.setBasePath("human", "male")
  shared.setAllPaths("knight")

  local attributes = shared.defaultSkeletonAttributes()
  attributes["neckHeight"] = 0.6
  attributes["headHeight"] = 11.0
  return attributes
end
