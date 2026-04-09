local module = {}

--- Parse face string to axis index (0=X, 1=Y, 2=Z)
--- @param face string face direction string
--- @return number axis index
function module.faceToAxisIndex(face)
    if face:find("X") or face:find("x") then
        return 0
    end
    if face:find("Y") or face:find("y") then
        return 1
    end
    if face:find("Z") or face:find("z") then
        return 2
    end
    return 1
end

--- Check if face is a positive direction
--- @param face string face direction string
--- @return boolean
function module.isPositiveFace(face)
    return face:find("Positive") ~= nil
end

--- Get position component by axis index
--- @param pos table|userdata position with x, y, z fields
--- @param ax number axis index (0=X, 1=Y, 2=Z)
--- @return number component value
function module.getAxis(pos, ax)
    if ax == 0 then
        return pos.x
    elseif ax == 1 then
        return pos.y
    else
        return pos.z
    end
end

--- Get component from an array-style position {x, y, z} by axis index
--- @param pos table position as {x, y, z} array
--- @param ax number axis index (0=X, 1=Y, 2=Z)
--- @return number component value
function module.getAxisArray(pos, ax)
    return pos[ax + 1]
end

--- Set position component by axis index
--- @param pos table|userdata position with x, y, z fields
--- @param ax number axis index (0=X, 1=Y, 2=Z)
--- @param val number value to set
function module.setAxis(pos, ax, val)
    if ax == 0 then
        pos.x = val
    elseif ax == 1 then
        pos.y = val
    else
        pos.z = val
    end
end

--- Check if a point is within the region bounds
--- @param rmin table region mins with x, y, z fields
--- @param rmax table region maxs with x, y, z fields
--- @param x number
--- @param y number
--- @param z number
--- @return boolean
function module.containsPoint(rmin, rmax, x, y, z)
    return x >= rmin.x and x <= rmax.x and y >= rmin.y and y <= rmax.y and z >= rmin.z and z <= rmax.z
end

--- Create a unique hash key for a 3D position
--- @param x number
--- @param y number
--- @param z number
--- @return number
function module.posKey(x, y, z)
    return x * 1048576 + y * 1024 + z
end

--- Build 4 neighbor offsets in a UV plane
--- @param uAxis number U axis index (0=X, 1=Y, 2=Z)
--- @param vAxis number V axis index (0=X, 1=Y, 2=Z)
--- @return table array of 4 offset tables {dx, dy, dz}
function module.buildPlaneOffsets(uAxis, vAxis)
    local offsets = {}
    for i = 1, 4 do
        offsets[i] = {0, 0, 0}
    end
    offsets[1][uAxis + 1] = 1
    offsets[2][uAxis + 1] = -1
    offsets[3][vAxis + 1] = 1
    offsets[4][vAxis + 1] = -1
    return offsets
end

--- Force a coordinate along a given axis to a fixed value
--- @param x number
--- @param y number
--- @param z number
--- @param wAxis number axis index to fix (0=X, 1=Y, 2=Z)
--- @param planeW number value to force on that axis
--- @return number, number, number adjusted coordinates
function module.forceAxis(x, y, z, wAxis, planeW)
    if wAxis == 0 then
        x = planeW
    elseif wAxis == 1 then
        y = planeW
    else
        z = planeW
    end
    return x, y, z
end

return module
