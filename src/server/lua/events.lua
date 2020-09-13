function init()
  local events = {
    "generic"
  }
  for key, value in ipairs(events) do
    local name = "event." .. value
    local mod = require(name)
    mod.register()
  end
end
