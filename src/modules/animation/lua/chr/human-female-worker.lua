require 'chr.bones'
require 'chr.shared'

function init()
  setupMeshTypes()
  setupBones()
  setBasePath("human", "female")
  setAllPaths("worker")
  settings.setPath("shoulder", "")

  return defaultSkeletonAttributes()
end
