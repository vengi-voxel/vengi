require 'chr.bones'
require 'chr.shared'

function init()
  setupMeshTypes()
  setupBones()
  setBasePath("human", "male")
  setAllPaths("knight")

  local attributes = defaultSkeletonAttributes()
  attributes["neckHeight"] = 0.6
  attributes["headHeight"] = 11.0
  return attributes
end
