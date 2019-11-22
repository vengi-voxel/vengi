-- bone.setup("meshtype")
-- bone.add("bonename", <mirrored>)

function setupBones()
  local head = bone.setup("head")
  head:add("head")

  local torso = bone.setup("chest")
  torso:add("chest")

  local foot = bone.setup("belt")
  foot:add("belt")

  local pants = bone.setup("pants")
  pants:add("pants")

  local hand = bone.setup("hand")
  hand:add("righthand")
  hand:add("lefthand", true)

  local foot = bone.setup("foot")
  foot:add("rightfoot")
  foot:add("leftfoot", true)

  local shoulder = bone.setup("shoulder")
  shoulder:add("rightshoulder")
  shoulder:add("leftshoulder", true)

  local glider = bone.setup("glider")
  glider:add("glider")
end
