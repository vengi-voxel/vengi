local bones = require 'chr.bones'
local shared = require 'chr.shared'

function init()
  shared.setupMeshTypes()
  bones.setupBones()
  shared.setBasePath("human", "male")
  shared.setAllPaths("blacksmith")

  return shared.defaultSkeletonAttributes()
end
