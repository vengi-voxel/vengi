function setupBones()
  local head = bone.setup("head")
  head:add("head")

  local torso = bone.setup("body")
  torso:add("torso")

  local hand = bone.setup("wing")
  hand:add("rightwing")
  hand:add("leftwing", true)

  local foot = bone.setup("foot")
  foot:add("rightfoot")
  foot:add("leftfoot", true)
end
