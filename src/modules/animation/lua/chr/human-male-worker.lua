require 'chr.bones'
require 'chr.shared'

function init()
  setupMeshTypes()
  setupBones()
  setBasePath("human", "male")
  settings.setPath("head", "worker")
  settings.setPath("belt", "worker")
  settings.setPath("chest", "worker")
  settings.setPath("pants", "worker")
  settings.setPath("hand", "worker")
  settings.setPath("foot", "worker")
  settings.setPath("shoulder", "")
end
