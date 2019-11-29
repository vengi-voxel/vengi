require 'chr.bones'
require 'chr.shared'

function init()
  setupMeshTypes()
  setupBones()
  setBasePath("human", "male")
  setAllPaths("shepherd")

  return defaultSkeletonAttributes()
end
