require 'chr.bones'
require 'chr.shared'

function init()
  setupMeshTypes()
  setupBones()
  setBasePath("human", "male")
  setAllPaths("worker")
  settings.setPath("shoulder", "")

  return defaultSkeletonAttributes()
end
