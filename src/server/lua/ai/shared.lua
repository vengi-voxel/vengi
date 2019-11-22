function increasePopulation (parentnode)
  local parallel = parentnode:addNode("Parallel", "increasepopulation")
  parallel:setCondition("And(Not(IsOnCooldown{INCREASE}),Filter(SelectIncreasePartner{INCREASE}))")

  parallel:addNode("Steer(SelectionSeek)", "followincreasepartner")
  local spawn = parallel:addNode("Parallel", "spawn")
  spawn:setCondition("IsCloseToSelection{1}")

  spawn:addNode("Spawn", "spawn")
  spawn:addNode("TriggerCooldown{INCREASE}", "increasecooldown")
  spawn:addNode("TriggerCooldownOnSelection{INCREASE}", "increasecooldownonpartner")
end

function hunt (parentnode)
  local parallel = parentnode:addNode("Parallel", "hunt")
  parallel:setCondition("And(Not(IsOnCooldown{HUNT}),Filter(SelectEntitiesOfTypes{ANIMAL_RABBIT}))")

  parallel:addNode("Steer(SelectionSeek)", "follow")
  parallel:addNode("AttackOnSelection", "attack"):setCondition("IsCloseToSelection{1}")
  parallel:addNode("SetPointOfInterest", "setpoi"):setCondition("IsCloseToSelection{1}")
  parallel:addNode("TriggerCooldown{HUNT}", "increasecooldown"):setCondition("Not(IsSelectionAlive)")
end

function idle (parentnode)
  local prio = parentnode:addNode("PrioritySelector", "walkuncrowded")

  prio:addNode("Steer(Wander)", "wanderfreely")
end

function idlehome (parentnode)
  local prio = parentnode:addNode("PrioritySelector", "walkuncrowded")

  -- if there are too many objects (see parameter) visible of either the same npc type or the max count, leave the area
  -- otherwise walk randomly around in the area around your home position
  prio:addNode("Steer(WanderAroundHome{100})", "wanderathome") --:addCondition("Not(IsCrowded{10, 100})")
  -- if we can't walk in our home base area, we are wandering freely around to find another not crowded area
  prio:addNode("Steer(Wander)", "wanderfreely")
end

function die (parentnode)
end
