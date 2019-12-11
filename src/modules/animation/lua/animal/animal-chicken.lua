require 'animal.bird-bones'
require 'animal.bird-shared'

function init()
  settings.setMeshTypes("head", "foot", "wing", "body")
  setupBones()
  setBasePath("chicken")

  return defaultSkeletonAttributes()
end
