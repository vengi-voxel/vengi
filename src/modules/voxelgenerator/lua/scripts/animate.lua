local function animateHead(node, frame)
    local kf = node:addKeyFrame(frame)
    kf:setLocalTranslation(0, 0.1 * math.sin(frame * 0.1), 0)
end

function main(node, region, color, padding)
    local allNodeIds = g_scenegraph.nodeIds()
    for frame = 1, 100 do
        for i, nodeId in ipairs(allNodeIds) do
            local node2 = g_scenegraph.get(nodeId)
            if node2:isModel() or node2:isReference() then
                if node2:name() == "unnamed" then
                    animateHead(node2, frame)
                end
            end
        end
    end
    g_scenegraph.updateTransforms()
end