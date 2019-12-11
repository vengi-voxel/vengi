function setupBones()
  local head = bone.setup("head")
  head:add("head")

  local body = bone.setup("body")
  body:add("body")

  local wing = bone.setup("wing")
  wing:add("rightwing")
  wing:add("leftwing", true)

  local foot = bone.setup("foot")
  foot:add("rightfoot")
  foot:add("leftfoot", true)
end
