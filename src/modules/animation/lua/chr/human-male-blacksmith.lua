require 'chr.bones'
require 'chr.shared'

function init()
  setupMeshTypes()
  setupBones()
  setBasePath("human", "male")
  setAllPaths("blacksmith")

  return defaultSkeletonAttributes()
end
