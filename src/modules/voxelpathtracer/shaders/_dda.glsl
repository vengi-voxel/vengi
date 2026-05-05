// Voxel traversal with sparse brick pool + empty-block skipping.
// u_indirection: values < 4 are log2 empty-skip levels; values >= 4 are brickId + 4.
// u_bricks: 3D atlas of 8^3 bricks packed X then Y then Z (u_bricksperrow x u_brickspercol x layers).

uint fetchVoxelIndex(ivec3 cell) {
	ivec3 brick = cell / BrickSize;
	uint entry = texelFetch(u_indirection, brick, 0).r;
	if (entry < 4u) {
		return 0u;
	}
	int brickIndex = int(entry - 4u);
	ivec3 local = cell - brick * BrickSize;
	int bx = brickIndex % u_bricksperrow;
	int tmp = brickIndex / u_bricksperrow;
	int by = tmp % u_brickspercol;
	int bz = tmp / u_brickspercol;
	ivec3 coord = ivec3(bx * BrickSize + local.x, by * BrickSize + local.y, bz * BrickSize + local.z);
	return texelFetch(u_bricks, coord, 0).r;
}

bool indexTransmits(uint index) {
	return payloadTransmits(index);
}

bool isOutside(ivec3 cell) {
	return any(lessThan(cell, ivec3(0))) || any(greaterThanEqual(cell, u_volumesize));
}

bool cellOccupied(ivec3 cell) {
	if (isOutside(cell)) {
		return false;
	}
	return fetchVoxelIndex(cell) != 0u;
}

// Neighbor-aware rounded box. Faces stay flat; only exposed edges and corners fillet.
// Adapted from Inigo Quilez's rounded-box signed distance function:
// https://iquilezles.org/articles/distfunctions/
float roundedBoxSdf(vec3 localPos, ivec3 cell) {
	vec3 fromCenter = localPos - (vec3(cell) + 0.5);
	float maxRadius = clamp(u_edgeradius, 0.0, 1.0) * 0.49;
	ivec3 octant = ivec3(greaterThanEqual(fromCenter, vec3(0.0))) * 2 - ivec3(1);
	int emptyAxes = 0;
	emptyAxes += cellOccupied(cell + ivec3(octant.x, 0, 0)) ? 0 : 1;
	emptyAxes += cellOccupied(cell + ivec3(0, octant.y, 0)) ? 0 : 1;
	emptyAxes += cellOccupied(cell + ivec3(0, 0, octant.z)) ? 0 : 1;
	float radius = emptyAxes >= 2 ? maxRadius : 0.0;
	vec3 box = abs(fromCenter) - vec3(0.5) + radius;
	return length(max(box, 0.0)) + min(max(box.x, max(box.y, box.z)), 0.0) - radius;
}

bool intersectRoundedVoxel(vec3 localOrigin, vec3 rayDir, ivec3 cell, float startDist, out float hitDist) {
	float dist = startDist;
	for (int i = 0; i < 20; i++) {
		vec3 pos = localOrigin + rayDir * dist;
		vec3 local = pos - vec3(cell);
		if (any(lessThan(local, vec3(-0.05))) || any(greaterThan(local, vec3(1.05)))) {
			return false;
		}
		float sdf = roundedBoxSdf(pos, cell);
		if (sdf < 0.002) {
			hitDist = dist;
			return true;
		}
		dist += max(sdf, 0.001);
	}
	return false;
}

// Amanatides-Woo 3D DDA, extended with hierarchical empty-block skipping over the
// sparse indirection grid.
// "A Fast Voxel Traversal Algorithm for Ray Tracing", Eurographics 1987:
// https://doi.org/10.2312/egtp.19871000
bool traceVoxelsDda(vec3 rayOrigin, vec3 rayDir, float maxDist, bool insideMedium, out float hitDist, out ivec3 hitCell,
					out uint hitIndex) {
	hitDist = 0.0;
	hitCell = ivec3(0);
	hitIndex = 0u;
	if (u_volumeactive == 0) {
		return false;
	}

	vec3 localOrigin = rayOrigin - vec3(u_volumemin);
	vec3 invDir = 1.0 / rayDir;
	vec3 distToMinPlanes = -localOrigin * invDir;
	vec3 distToMaxPlanes = (vec3(u_volumesize) - localOrigin) * invDir;
	vec3 axisEnterDist = min(distToMinPlanes, distToMaxPlanes);
	vec3 axisExitDist = max(distToMinPlanes, distToMaxPlanes);
	float enterDist = max(max(max(axisEnterDist.x, axisEnterDist.y), axisEnterDist.z), 0.0);
	float exitDist = min(min(min(axisExitDist.x, axisExitDist.y), axisExitDist.z), maxDist);
	if (enterDist > exitDist) {
		return false;
	}

	float dist = enterDist;
	// Relative epsilon: a fixed 1e-4 undershoots at large dist and can leave the
	// start cell on the entry face of an empty brick.
	float startDist = enterDist + 1e-5 * (1.0 + abs(enterDist));
	ivec3 cell = clamp(ivec3(floor(localOrigin + rayDir * startDist)), ivec3(0), u_volumesize - 1);
	ivec3 cellStep = ivec3(sign(rayDir));
	vec3 deltaDist = abs(invDir);
	vec3 nextDist = (vec3(cell) + max(vec3(cellStep), vec3(0.0)) - localOrigin) * invDir;

	ivec3 cachedBrick = ivec3(-1);
	uint cachedEntry = 255u;
	for (int i = 0; i < MaxSteps; i++) {
		if (!insideMedium) {
			ivec3 brick = cell / BrickSize;
			// Reuse indirection while traversing voxels in the same brick.
			if (any(notEqual(brick, cachedBrick))) {
				cachedBrick = brick;
				cachedEntry = texelFetch(u_indirection, brick, 0).r;
			}
			if (cachedEntry < 4u) {
				// Snap only the crossing axis to the next block. Adding an epsilon here
				// can skip thin voxels; the other axes must follow the ray intersection.
				int shift = int(log2(float(BrickSize))) + int(cachedEntry);
				int emptyBlockSize = 1 << shift;
				vec3 brickMin = vec3((cell >> shift) << shift);
				vec3 exitPlane = brickMin + vec3(greaterThanEqual(rayDir, vec3(0.0))) * float(emptyBlockSize);
				vec3 brickExit = (exitPlane - localOrigin) * invDir;
				float brickExitDist = min(min(brickExit.x, brickExit.y), brickExit.z);
				if (brickExitDist > dist) {
					ivec3 nextCell = ivec3(floor(localOrigin + rayDir * brickExitDist));
					if (brickExit.x <= brickExit.y && brickExit.x <= brickExit.z) {
						nextCell.x = int(exitPlane.x) - (rayDir.x < 0.0 ? 1 : 0);
					} else if (brickExit.y <= brickExit.z) {
						nextCell.y = int(exitPlane.y) - (rayDir.y < 0.0 ? 1 : 0);
					} else {
						nextCell.z = int(exitPlane.z) - (rayDir.z < 0.0 ? 1 : 0);
					}
					dist = brickExitDist;
					if (dist > exitDist || isOutside(nextCell)) {
						return false;
					}
					if (any(notEqual(nextCell, cell))) {
						cell = nextCell;
						nextDist = (vec3(cell) + max(vec3(cellStep), vec3(0.0)) - localOrigin) * invDir;
						continue;
					}
				}
			}
		}

		uint voxelIndex = fetchVoxelIndex(cell);
		bool boundary = insideMedium ? (voxelIndex == 0u || !indexTransmits(voxelIndex)) : (voxelIndex != 0u);
		if (boundary) {
			if (!insideMedium && u_edgeradius > 1e-4 && voxelIndex != 0u) {
				float roundedDist = dist;
				if (intersectRoundedVoxel(localOrigin, rayDir, cell, dist, roundedDist)) {
					hitDist = roundedDist;
					hitCell = cell;
					hitIndex = voxelIndex;
					return true;
				}
			} else {
				hitDist = dist;
				hitCell = cell;
				hitIndex = voxelIndex;
				return true;
			}
		}

		// Advance exactly one axis, with z, y, x priority for ties.
		bool stepX = nextDist.x < nextDist.y && nextDist.x < nextDist.z;
		bool stepY = !stepX && nextDist.y < nextDist.z;
		bvec3 stepAxis = bvec3(stepX, stepY, !stepX && !stepY);
		dist = min(nextDist.x, min(nextDist.y, nextDist.z));
		nextDist += vec3(stepAxis) * deltaDist;
		cell += ivec3(stepAxis) * cellStep;

		if (dist > exitDist || isOutside(cell)) {
			if (insideMedium) {
				hitDist = dist;
				hitCell = cell;
				hitIndex = 0u;
				return true;
			}
			return false;
		}
	}

	return false;
}

#ifdef VENGIRAYQUERY
// Hardware BVH traversal over occupied 8x8x8 brick AABBs via VK_KHR_ray_query,
// with a tight 8x8x8 local DDA inside each candidate brick directly on u_bricks.
bool traceVoxels(vec3 rayOrigin, vec3 rayDir, float maxDist, bool insideMedium, out float hitDist, out ivec3 hitCell,
				 out uint hitIndex) {
	if (insideMedium) {
		return traceVoxelsDda(rayOrigin, rayDir, maxDist, true, hitDist, hitCell, hitIndex);
	}
	hitDist = 0.0;
	hitCell = ivec3(0);
	hitIndex = 0u;
	if (u_volumeactive == 0) {
		return false;
	}

	vec3 localOrigin = rayOrigin - vec3(u_volumemin);
	vec3 invDir = 1.0 / rayDir;
	vec3 distToMinPlanes = -localOrigin * invDir;
	vec3 distToMaxPlanes = (vec3(u_volumesize) - localOrigin) * invDir;
	vec3 axisEnterDist = min(distToMinPlanes, distToMaxPlanes);
	vec3 axisExitDist = max(distToMinPlanes, distToMaxPlanes);
	float enterDist = max(max(max(axisEnterDist.x, axisEnterDist.y), axisEnterDist.z), 0.0);
	float exitDist = min(min(min(axisExitDist.x, axisExitDist.y), axisExitDist.z), maxDist);
	if (enterDist > exitDist) {
		return false;
	}

	float tMin = max(enterDist - 1e-4, 0.0);
	float tMax = exitDist + 1e-4;
	ivec3 cellStep = ivec3(sign(rayDir));
	vec3 deltaDist = abs(invDir);

	bool found = false;
	float bestDist = exitDist;
	ivec3 bestCell = ivec3(0);
	uint bestIndex = 0u;

	rayQueryEXT rq;
	rayQueryInitializeEXT(rq, u_tlas, gl_RayFlagsNoneEXT, 0xFFu, localOrigin, tMin, rayDir, tMax);
	while (rayQueryProceedEXT(rq)) {
		if (rayQueryGetIntersectionTypeEXT(rq, false) == gl_RayQueryCandidateIntersectionAABBEXT) {
			int brickIndex = rayQueryGetIntersectionPrimitiveIndexEXT(rq, false);
			vec3 brickCoordF = texelFetch(u_brickcoords, ivec2(brickIndex & 1023, brickIndex >> 10), 0).xyz;
			ivec3 brickCoord = ivec3(brickCoordF + 0.5);
			ivec3 brickBase = brickCoord * BrickSize;
			vec3 bMin = vec3(brickBase);
			vec3 bMax = bMin + vec3(BrickSize);
			vec3 t0 = (bMin - localOrigin) * invDir;
			vec3 t1 = (bMax - localOrigin) * invDir;
			vec3 tSmall = min(t0, t1);
			vec3 tBig = max(t0, t1);
			float bEnter = max(max(max(tSmall.x, tSmall.y), tSmall.z), enterDist);
			float bExit = min(min(min(tBig.x, tBig.y), tBig.z), bestDist);
			if (bEnter > bExit) {
				continue;
			}

			int abx = brickIndex % u_bricksperrow;
			int atmp = brickIndex / u_bricksperrow;
			int aby = atmp % u_brickspercol;
			int abz = atmp / u_brickspercol;
			ivec3 atlasBase = ivec3(abx, aby, abz) * BrickSize;

			float dist = bEnter;
			float startDist = bEnter + 1e-5 * (1.0 + abs(bEnter));
			ivec3 localCell =
				clamp(ivec3(floor(localOrigin + rayDir * startDist)) - brickBase, ivec3(0), ivec3(BrickSize - 1));
			ivec3 cell = brickBase + localCell;
			vec3 nextDist = (vec3(cell) + max(vec3(cellStep), vec3(0.0)) - localOrigin) * invDir;

			for (int stepIdx = 0; stepIdx < BrickSize * 3; ++stepIdx) {
				if (!isOutside(cell)) {
					uint voxelIndex = texelFetch(u_bricks, atlasBase + localCell, 0).r;
					if (voxelIndex != 0u) {
						float candidateDist = dist;
						bool accepted = true;
						if (u_edgeradius > 1e-4) {
							accepted = intersectRoundedVoxel(localOrigin, rayDir, cell, dist, candidateDist);
						}
						if (accepted && candidateDist <= bestDist) {
							bestDist = candidateDist;
							bestCell = cell;
							bestIndex = voxelIndex;
							found = true;
							float commitT = clamp(candidateDist, tMin, tMax);
							rayQueryGenerateIntersectionEXT(rq, commitT);
							break;
						}
					}
				}
				bool stepX = nextDist.x < nextDist.y && nextDist.x < nextDist.z;
				bool stepY = !stepX && nextDist.y < nextDist.z;
				bvec3 stepAxis = bvec3(stepX, stepY, !stepX && !stepY);
				dist = min(nextDist.x, min(nextDist.y, nextDist.z));
				nextDist += vec3(stepAxis) * deltaDist;
				ivec3 stepVec = ivec3(stepAxis) * cellStep;
				localCell += stepVec;
				cell += stepVec;
				if (dist > bExit || any(lessThan(localCell, ivec3(0))) ||
					any(greaterThanEqual(localCell, ivec3(BrickSize)))) {
					break;
				}
			}
		}
	}

	if (found) {
		hitDist = bestDist;
		hitCell = bestCell;
		hitIndex = bestIndex;
		return true;
	}
	return false;
}
#else
bool traceVoxels(vec3 rayOrigin, vec3 rayDir, float maxDist, bool insideMedium, out float hitDist, out ivec3 hitCell,
				 out uint hitIndex) {
	return traceVoxelsDda(rayOrigin, rayDir, maxDist, insideMedium, hitDist, hitCell, hitIndex);
}
#endif

vec3 avoidZero(vec3 dir) {
	return vec3(abs(dir.x) < 1e-6 ? 1e-6 : dir.x, abs(dir.y) < 1e-6 ? 1e-6 : dir.y, abs(dir.z) < 1e-6 ? 1e-6 : dir.z);
}

vec3 roundedVoxelNormal(vec3 worldPos, ivec3 cell) {
	vec3 localPos = worldPos - vec3(u_volumemin);
	const float eps = 0.002;
	vec3 normal = vec3(
		roundedBoxSdf(localPos + vec3(eps, 0.0, 0.0), cell) - roundedBoxSdf(localPos - vec3(eps, 0.0, 0.0), cell),
		roundedBoxSdf(localPos + vec3(0.0, eps, 0.0), cell) - roundedBoxSdf(localPos - vec3(0.0, eps, 0.0), cell),
		roundedBoxSdf(localPos + vec3(0.0, 0.0, eps), cell) - roundedBoxSdf(localPos - vec3(0.0, 0.0, eps), cell));
	float normalLength = length(normal);
	if (normalLength < 1e-6) {
		return vec3(0.0, 1.0, 0.0);
	}
	return normal / normalLength;
}

vec3 faceNormal(vec3 rayOrigin, vec3 rayDir, ivec3 cell) {
	vec3 localOrigin = rayOrigin - vec3(u_volumemin);
	vec3 cellMin = vec3(cell);
	vec3 entry = cellMin + vec3(lessThan(rayDir, vec3(0.0)));
	vec3 faceDist = (entry - localOrigin) / rayDir;
	if (faceDist.x > faceDist.y && faceDist.x > faceDist.z) {
		return vec3(-sign(rayDir.x), 0.0, 0.0);
	}
	if (faceDist.y > faceDist.z) {
		return vec3(0.0, -sign(rayDir.y), 0.0);
	}
	return vec3(0.0, 0.0, -sign(rayDir.z));
}
