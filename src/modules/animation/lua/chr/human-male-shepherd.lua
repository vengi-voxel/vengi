require 'chr.bones'
require 'chr.shared'

function init()
  setupMeshTypes()
  setupBones()
  setBasePath("human", "male")
  settings.setPath("head", "shepherd")
  settings.setPath("belt", "shepherd")
  settings.setPath("chest", "shepherd")
  settings.setPath("pants", "shepherd")
  settings.setPath("hand", "shepherd")
  settings.setPath("foot", "shepherd")
  settings.setPath("shoulder", "shepherd")
end
