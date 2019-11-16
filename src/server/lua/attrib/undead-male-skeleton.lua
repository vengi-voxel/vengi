require "attrib.shared"

function registerSkeleton()
	local chr = attrib.createContainer("UNDEAD_MALE_SKELETON")
	characterDefault(chr)
	chr:register()
end

