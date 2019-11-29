require 'chr.bones'
require 'chr.shared'

function init()
  setupMeshTypes()
  setupBones()
  setBasePath("dwarf", "male")
  setAllPaths("blacksmith")

  return defaultSkeletonAttributes()
end
