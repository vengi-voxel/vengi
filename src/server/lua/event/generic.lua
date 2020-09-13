local module = {}

function module.register()
  local event = eventmgr.create("GENERIC", "GENERIC")
  --print(event:type())
  --print(event:name())
end

return module
