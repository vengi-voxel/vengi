require 'chr.bones'
require 'chr.shared'

function init()
  setupMeshTypes()
  setupBones()
  setBasePath("dwarf", "male")
  settings.setPath("head", "blacksmith")
  settings.setPath("belt", "blacksmith")
  settings.setPath("chest", "blacksmith")
  settings.setPath("pants", "blacksmith")
  settings.setPath("hand", "blacksmith")
  settings.setPath("foot", "blacksmith")
  settings.setPath("shoulder", "blacksmith")

  return defaultSkeletonAttributes()
end
