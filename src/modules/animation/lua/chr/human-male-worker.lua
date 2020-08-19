local bones = require 'chr.bones'
local shared = require 'chr.shared'

function init()
  shared.setupMeshTypes()
  bones.setupBones()
  shared.setBasePath("human", "male")
  shared.setAllPaths("worker")
  settings.setPath("shoulder", "")

  return shared.defaultSkeletonAttributes()
end
