/**
 * @file
 */

#include "KFAFormat.h"
#include "core/Common.h"
#include "core/FourCC.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/collection/Buffer.h"
#include "core/collection/Queue.h"
#include "io/Stream.h"
#include "palette/NormalPalette.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphTransform.h"
#include "voxel/Connectivity.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeCropper.h"
#include "voxelutil/VolumeSplitter.h"
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/matrix.hpp>

namespace voxelformat {

namespace kfa_priv {

constexpr uint32_t MAXSPRITES = 1024;
constexpr int32_t PI_UNITS = 32768;

struct KenSprite {
	glm::vec3 s{1.0f, 0.0f, 0.0f};
	glm::vec3 h{0.0f, 1.0f, 0.0f};
	glm::vec3 f{0.0f, 0.0f, 1.0f};
	glm::vec3 p{0.0f, 0.0f, 0.0f};
};

static glm::vec3 toVec(const KFAFormat::Point3 &p) {
	return glm::vec3(p.x, p.y, p.z);
}

// Ken sprite basis (s, h, f, p) as a 4x4 matrix. Local voxel (x,y,z) -> p + x*s + y*h + z*f
static glm::mat4 spriteMatrix(const KenSprite &spr) {
	glm::mat4 m(1.0f);
	m[0] = glm::vec4(spr.s, 0.0f);
	m[1] = glm::vec4(spr.h, 0.0f);
	m[2] = glm::vec4(spr.f, 0.0f);
	m[3] = glm::vec4(spr.p, 1.0f);
	return m;
}

// Ken (x, y, z) -> vengi (x, -z, y)
static glm::mat4 kenToVengi() {
	glm::mat4 m(0.0f);
	m[0] = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
	m[1] = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
	m[2] = glm::vec4(0.0f, -1.0f, 0.0f, 0.0f);
	m[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	return m;
}

static glm::mat4 vengiToKen() {
	glm::mat4 m(0.0f);
	m[0] = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
	m[1] = glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);
	m[2] = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
	m[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	return m;
}

// Given vector a, returns b and c that make (a, b, c) orthonormal - Ken's genperp
static void genperp(const glm::vec3 &a, glm::vec3 &b, glm::vec3 &c) {
	if (a.x == 0.0f && a.y == 0.0f && a.z == 0.0f) {
		b = glm::vec3(0.0f);
		c = glm::vec3(0.0f);
		return;
	}
	if (glm::abs(a.x) < glm::abs(a.y) && glm::abs(a.x) < glm::abs(a.z)) {
		const float t = 1.0f / glm::sqrt(a.y * a.y + a.z * a.z);
		b = glm::vec3(0.0f, a.z * t, -a.y * t);
	} else if (glm::abs(a.y) < glm::abs(a.z)) {
		const float t = 1.0f / glm::sqrt(a.x * a.x + a.z * a.z);
		b = glm::vec3(-a.z * t, 0.0f, a.x * t);
	} else {
		const float t = 1.0f / glm::sqrt(a.x * a.x + a.y * a.y);
		b = glm::vec3(a.y * t, -a.x * t, 0.0f);
	}
	c = glm::cross(a, b);
}

// A * B = C, find A (Ken mat0). Columns are s, h, f, origin.
static void mat0(glm::vec3 &as, glm::vec3 &ah, glm::vec3 &ag, glm::vec3 &ao, const glm::vec3 &bs, const glm::vec3 &bh,
				 const glm::vec3 &bg, const glm::vec3 &bo, const glm::vec3 &cs, const glm::vec3 &ch,
				 const glm::vec3 &cg, const glm::vec3 &co) {
	glm::vec3 ts, th, tg, to;
	ts.x = bs.x * cs.x + bh.x * ch.x + bg.x * cg.x;
	ts.y = bs.x * cs.y + bh.x * ch.y + bg.x * cg.y;
	ts.z = bs.x * cs.z + bh.x * ch.z + bg.x * cg.z;
	th.x = bs.y * cs.x + bh.y * ch.x + bg.y * cg.x;
	th.y = bs.y * cs.y + bh.y * ch.y + bg.y * cg.y;
	th.z = bs.y * cs.z + bh.y * ch.z + bg.y * cg.z;
	tg.x = bs.z * cs.x + bh.z * ch.x + bg.z * cg.x;
	tg.y = bs.z * cs.y + bh.z * ch.y + bg.z * cg.y;
	tg.z = bs.z * cs.z + bh.z * ch.z + bg.z * cg.z;
	to.x = co.x - bo.x * ts.x - bo.y * th.x - bo.z * tg.x;
	to.y = co.y - bo.x * ts.y - bo.y * th.y - bo.z * tg.y;
	to.z = co.z - bo.x * ts.z - bo.y * th.z - bo.z * tg.z;
	as = ts;
	ah = th;
	ag = tg;
	ao = to;
}

// A * B = C, find C (Ken mat2)
static void mat2(const glm::vec3 &as, const glm::vec3 &ah, const glm::vec3 &af, const glm::vec3 &ao,
				 const glm::vec3 &bs, const glm::vec3 &bh, const glm::vec3 &bf, const glm::vec3 &bo, glm::vec3 &cs,
				 glm::vec3 &ch, glm::vec3 &cf, glm::vec3 &co) {
	glm::vec3 ts, th, tf, to;
	ts.x = as.x * bs.x + ah.x * bs.y + af.x * bs.z;
	ts.y = as.y * bs.x + ah.y * bs.y + af.y * bs.z;
	ts.z = as.z * bs.x + ah.z * bs.y + af.z * bs.z;
	th.x = as.x * bh.x + ah.x * bh.y + af.x * bh.z;
	th.y = as.y * bh.x + ah.y * bh.y + af.y * bh.z;
	th.z = as.z * bh.x + ah.z * bh.y + af.z * bh.z;
	tf.x = as.x * bf.x + ah.x * bf.y + af.x * bf.z;
	tf.y = as.y * bf.x + ah.y * bf.y + af.y * bf.z;
	tf.z = as.z * bf.x + ah.z * bf.y + af.z * bf.z;
	to.x = as.x * bo.x + ah.x * bo.y + af.x * bo.z + ao.x;
	to.y = as.y * bo.x + ah.y * bo.y + af.y * bo.z + ao.y;
	to.z = as.z * bo.x + ah.z * bo.y + af.z * bo.z + ao.z;
	cs = ts;
	ch = th;
	cf = tf;
	co = to;
}

static void setlimb(core::Buffer<KenSprite> &spr, const KFAFormat::Hinge &hinge, int32_t child, int32_t parent,
					int8_t htype, int32_t val) {
	glm::vec3 qs = toVec(hinge.v[0]);
	glm::vec3 qp = toVec(hinge.p[0]);
	glm::vec3 qh, qf;
	genperp(qs, qh, qf);

	if (htype == 0) {
		const float angle = ((float)val) * (glm::pi<float>() / (float)PI_UNITS);
		const float c = glm::cos(angle);
		const float s = glm::sin(angle);
		const glm::vec3 ph = qh;
		const glm::vec3 pf = qf;
		qh = ph * c - pf * s;
		qf = ph * s + pf * c;
	}

	glm::vec3 ps = toVec(hinge.v[1]);
	glm::vec3 pp = toVec(hinge.p[1]);
	glm::vec3 ph, pf;
	genperp(ps, ph, pf);

	mat0(qs, qh, qf, qp, qs, qh, qf, qp, ps, ph, pf, pp);
	mat2(spr[parent].s, spr[parent].h, spr[parent].f, spr[parent].p, qs, qh, qf, qp, spr[child].s, spr[child].h,
		 spr[child].f, spr[child].p);
}

// Parents first (Ken evaluates hingesort from the end)
static bool sortHinges(const core::Buffer<KFAFormat::Hinge> &hinges, core::Buffer<int32_t> &evalOrder) {
	const int nh = (int)hinges.size();
	evalOrder.clear();
	if (nh <= 0) {
		return true;
	}

	core::Buffer<int32_t> parent;
	parent.reserve(nh);
	for (int i = 0; i < nh; ++i) {
		parent.push_back(hinges[i].parent);
	}

	core::Buffer<int32_t> hsort;
	hsort.resize(nh);
	int n = nh;
	int j = 0;
	for (int i = n - 1; i >= 0; --i) {
		if (parent[i] < 0) {
			hsort[--n] = i;
		} else {
			hsort[j++] = i;
		}
	}

	int i = 0;
	int guard = nh * nh + 8;
	while (n > 0 && guard-- > 0) {
		--i;
		if (i < 0) {
			i = n - 1;
		}
		j = hsort[i];
		const int32_t p = parent[j];
		if (p < 0 || p >= nh) {
			Log::error("KFA hinge %d has invalid parent %d", j, p);
			return false;
		}
		if (parent[p] < 0) {
			parent[j] = -2 - parent[j];
			--n;
			hsort[i] = hsort[n];
			hsort[n] = j;
		}
	}
	if (n > 0) {
		Log::error("KFA hinge parent links form a cycle");
		return false;
	}

	evalOrder.reserve(nh);
	for (int idx = nh - 1; idx >= 0; --idx) {
		evalOrder.push_back(hsort[idx]);
	}
	return true;
}

static void evaluatePose(const KFAFormat::Data &data, const core::Buffer<glm::vec3> &restP,
						 const core::Buffer<int32_t> &evalOrder, const int32_t *angles,
						 core::Buffer<KenSprite> &sprites) {
	const uint32_t numhin = (uint32_t)data.hinges.size();
	sprites.clear();
	sprites.reserve(numhin);
	for (uint32_t i = 0; i < numhin; ++i) {
		KenSprite spr;
		if (i < restP.size()) {
			spr.p = restP[i];
		}
		sprites.push_back(spr);
	}

	for (int32_t hingeIdx : evalOrder) {
		const KFAFormat::Hinge &hinge = data.hinges[hingeIdx];
		if (hinge.parent < 0) {
			continue;
		}
		const int32_t val = angles != nullptr ? angles[hingeIdx] : 0;
		setlimb(sprites, hinge, hingeIdx, hinge.parent, hinge.htype, val);
	}
}

static void matrixToLocal(const glm::mat4 &m, scenegraph::SceneGraphTransform &transform) {
	const glm::vec3 t(m[3]);
	glm::vec3 x(m[0]);
	glm::vec3 y(m[1]);
	glm::vec3 z(m[2]);
	glm::vec3 scale(glm::length(x), glm::length(y), glm::length(z));
	if (scale.x < 1e-8f) {
		scale.x = 1.0f;
	}
	if (scale.y < 1e-8f) {
		scale.y = 1.0f;
	}
	if (scale.z < 1e-8f) {
		scale.z = 1.0f;
	}
	x /= scale.x;
	y /= scale.y;
	z /= scale.z;
	glm::mat3 rot(x, y, z);
	transform.setLocalTranslation(t);
	transform.setLocalOrientation(glm::normalize(glm::quat_cast(rot)));
	transform.setLocalScale(scale);
}

// Ken local offset -> vengi offset (x, y, z) -> (x, -z, y)
static glm::vec3 kenToVengiOffset(const glm::vec3 &ken) {
	return glm::vec3(ken.x, -ken.z, ken.y);
}

// Hinge velcro in volume voxel coordinates (0-based after the limb is shifted to origin)
static glm::vec3 hingeVolumePos(const glm::vec3 &volumeOrigin, const glm::vec3 &hingeKen) {
	return volumeOrigin + kenToVengiOffset(hingeKen);
}

// World matrix whose translation is the hinge. Combined with pivot = hingeVolume / dims
// the renderer (wm * T(-pivot * dims)) still places voxels at Ken's sprite transform.
// Bones use worldTranslation(), so the origin must be the velcro, not volume (0,0,0).
static glm::mat4 vengiWorldFromSprite(const KenSprite &spr, const glm::vec3 &volumeOrigin, const glm::vec3 &hingeKen,
									  bool hasVolume) {
	const glm::mat4 L = kenToVengi();
	const glm::mat4 Linv = vengiToKen();
	glm::mat4 m = L * spriteMatrix(spr) * Linv;
	const glm::vec3 hingeVolume = hingeVolumePos(hasVolume ? volumeOrigin : glm::vec3(0.0f), hingeKen);
	if (hasVolume) {
		m = m * glm::translate(glm::mat4(1.0f), -volumeOrigin);
	}
	m = m * glm::translate(glm::mat4(1.0f), hingeVolume);
	return m;
}

static const int16_t *frameAngles(const KFAFormat::Data &data, int32_t frame) {
	if (data.numFrames == 0 || data.hinges.empty()) {
		return nullptr;
	}
	if (frame < 0 || (uint32_t)frame >= data.numFrames) {
		return nullptr;
	}
	return &data.frmval[(size_t)frame * data.hinges.size()];
}

static void collectKeyedTimes(const KFAFormat::Data &data, core::Buffer<int32_t> &times,
							  core::Buffer<int32_t> &frames) {
	times.clear();
	frames.clear();
	bool wroteFrameZero = false;
	for (const KFAFormat::Seq &seq : data.seq) {
		// Negative seq.frame is ~target (loop/stop), not a pose
		if (seq.frame < 0) {
			continue;
		}
		if (KFAFormat::timeToFrame(seq.time) == 0) {
			wroteFrameZero = true;
		}
		times.push_back(seq.time);
		frames.push_back(seq.frame);
	}
	if (!wroteFrameZero) {
		times.push_back(0);
		frames.push_back(0);
	}
}

// Ken animsprite wrap-lerp of i16 hinge angles (vmin/vmax range, 65536 = 360deg)
static void lerpHingeAngles(const KFAFormat::Data &data, int32_t frame0, int32_t frame1, float t,
							core::Buffer<int32_t> &out) {
	out.clear();
	const uint32_t nh = (uint32_t)data.hinges.size();
	const int16_t *a0 = frameAngles(data, frame0);
	const int16_t *a1 = frameAngles(data, frame1);
	if (t < 0.0f) {
		t = 0.0f;
	} else if (t > 1.0f) {
		t = 1.0f;
	}
	out.reserve(nh);
	for (uint32_t i = 0; i < nh; ++i) {
		const int32_t frm0 = a0 != nullptr ? (int32_t)a0[i] : 0;
		if (a1 == nullptr || frame0 == frame1 || t <= 0.0f) {
			out.push_back(frm0);
			continue;
		}
		if (t >= 1.0f) {
			out.push_back((int32_t)a1[i]);
			continue;
		}
		int32_t x = ((int32_t)a1[i] - frm0) & 65535;
		const KFAFormat::Hinge &hinge = data.hinges[i];
		if (hinge.vmin == hinge.vmax) {
			x = (x << 16) >> 16;
		} else if ((((int32_t)a1[i] - (int32_t)hinge.vmin) & 65535) < ((frm0 - (int32_t)hinge.vmin) & 65535)) {
			x -= 65536;
		}
		out.push_back(frm0 + (int32_t)((float)x * t));
	}
}

static void freeVolumes(core::Buffer<voxel::RawVolume *> &volumes) {
	for (voxel::RawVolume *v : volumes) {
		delete v;
	}
	volumes.clear();
}

// Ken (x, y, z) -> vengi voxel in the loaded KV6 region
static glm::ivec3 kenVoxelToVengi(int kenX, int kenY, int kenZ, const voxel::Region &region) {
	return glm::ivec3(region.getLowerX() + kenX, region.getUpperY() - kenZ, region.getLowerZ() + kenY);
}

struct KenFifoEntry {
	int32_t x = 0;
	int32_t y = 0;
	int32_t v0 = 0;
	int32_t v1 = 0;
};

static core::Buffer<voxel::RawVolume *> splitKenVisSlabs(const voxel::RawVolume *volume,
														 const core::Buffer<KFAFormat::KenSpriteVox> &kenVoxels) {
	core::Buffer<voxel::RawVolume *> limbs;
	if (volume == nullptr || kenVoxels.empty()) {
		return limbs;
	}

	const voxel::Region &region = volume->region();
	const int xsiz = region.getWidthInVoxels();
	const int ysiz = region.getDepthInVoxels();
	if (xsiz <= 0 || ysiz <= 0) {
		return limbs;
	}

	const int32_t nvox = (int32_t)kenVoxels.size();
	core::Buffer<uint8_t> vis;
	vis.reserve((size_t)nvox);
	for (int32_t i = 0; i < nvox; ++i) {
		uint8_t v = kenVoxels[i].vis;
		v |= (uint8_t)((v & 32u) << 1); // Ken: vis |= (vis&32)<<1
		vis.push_back(v);
	}

	core::Buffer<int32_t> colStart;
	core::Buffer<int32_t> colCount;
	const size_t ncol = (size_t)xsiz * (size_t)ysiz;
	colStart.reserve(ncol);
	colCount.reserve(ncol);
	for (size_t i = 0; i < ncol; ++i) {
		colStart.push_back(-1);
		colCount.push_back(0);
	}
	for (int32_t i = 0; i < nvox; ++i) {
		const int x = (int)kenVoxels[i].x;
		const int y = (int)kenVoxels[i].y;
		if (x < 0 || x >= xsiz || y < 0 || y >= ysiz) {
			continue;
		}
		const int col = x * ysiz + y;
		if (colStart[col] < 0) {
			colStart[col] = i;
		}
		++colCount[col];
	}

	auto extractMarked = [&]() -> voxel::RawVolume * {
		voxel::RawVolume object(region);
		bool any = false;
		for (int32_t i = 0; i < nvox; ++i) {
			if ((vis[i] & 128u) == 0u) {
				continue;
			}
			vis[i] = (uint8_t)(vis[i] & ~128u);
			const glm::ivec3 p = kenVoxelToVengi(kenVoxels[i].x, kenVoxels[i].y, kenVoxels[i].z, region);
			if (region.containsPoint(p)) {
				object.setVoxel(p, volume->voxel(p));
				any = true;
			}
		}
		if (!any) {
			return nullptr;
		}
		if (voxel::RawVolume *cropped = voxelutil::cropVolume(&object)) {
			return cropped;
		}
		return new voxel::RawVolume(object);
	};

	auto floodOne = [&](int32_t ox, int32_t oy, int32_t v0, int32_t v1) -> bool {
		vis[v1] = (uint8_t)(vis[v1] & ~64u);
		core::Queue<KenFifoEntry> fifo;
		fifo.push(KenFifoEntry{ox, oy, v0, v1});
		int fifoCount = 1;
		constexpr int maxFifo = 4096; // Ken VFIFSIZ/4
		bool overflow = false;

		while (!fifo.empty()) {
			const KenFifoEntry e = fifo.pop();
			--fifoCount;
			ox = e.x;
			oy = e.y;
			v0 = e.v0;
			v1 = e.v1;
			for (int32_t i = v0; i <= v1; ++i) {
				vis[i] = (uint8_t)(vis[i] | 128u);
			}
			const int v0z = kenVoxels[v0].z;
			const int v1z = kenVoxels[v1].z;
			static const int32_t ndx[4] = {-1, 1, 0, 0};
			static const int32_t ndy[4] = {0, 0, -1, 1};
			for (int j = 0; j < 4; ++j) {
				const int x = ox + ndx[j];
				const int y = oy + ndy[j];
				if (x < 0 || x >= xsiz || y < 0 || y >= ysiz) {
					continue;
				}
				const int col = x * ysiz + y;
				const int32_t start = colStart[col];
				const int32_t count = colCount[col];
				if (start < 0 || count <= 0) {
					continue;
				}
				int32_t ov = -1;
				for (int32_t i = start; i < start + count; ++i) {
					if ((vis[i] & 16u) != 0u) {
						ov = i;
					}
					if (((vis[i] & (64u + 32u)) == (64u + 32u)) && ov >= 0 && v0z <= kenVoxels[i].z &&
						v1z >= kenVoxels[ov].z) {
						if (fifoCount >= maxFifo) {
							overflow = true;
							break;
						}
						fifo.push(KenFifoEntry{x, y, ov, i});
						++fifoCount;
						vis[i] = (uint8_t)(vis[i] & ~64u);
					}
				}
				if (overflow) {
					break;
				}
			}
			if (overflow) {
				break;
			}
		}

		if (overflow) {
			for (int32_t i = 0; i < nvox; ++i) {
				vis[i] = (uint8_t)(vis[i] | 128u);
				vis[i] = (uint8_t)(vis[i] & ~64u);
			}
			if (voxel::RawVolume *all = extractMarked()) {
				limbs.push_back(all);
			}
			return false;
		}
		if (voxel::RawVolume *limb = extractMarked()) {
			limbs.push_back(limb);
		}
		return true;
	};

	for (int x = 0; x < xsiz; ++x) {
		for (int y = 0; y < ysiz; ++y) {
			const int col = x * ysiz + y;
			const int32_t start = colStart[col];
			const int32_t count = colCount[col];
			if (start < 0 || count <= 0) {
				continue;
			}
			int32_t ov = -1;
			for (int32_t i = start; i < start + count; ++i) {
				if ((vis[i] & 16u) != 0u) {
					ov = i;
				}
				if ((vis[i] & (64u + 32u)) == (64u + 32u) && ov >= 0) {
					if (!floodOne(x, y, ov, i)) {
						return limbs;
					}
				}
			}
		}
	}
	return limbs;
}

} // namespace kfa_priv

#define wrap(read)                                                                                                     \
	if ((read) != 0) {                                                                                                 \
		Log::error("Could not load kfa file: Not enough data in stream " CORE_STRINGIFY(read));                        \
		return false;                                                                                                  \
	}
#define wrapBool(read)                                                                                                 \
	if ((read) == false) {                                                                                             \
		Log::error("Could not load kfa file: Not enough data in stream " CORE_STRINGIFY(read));                        \
		return false;                                                                                                  \
	}

scenegraph::FrameIndex KFAFormat::timeToFrame(int32_t timeMillis) {
	if (timeMillis <= 0) {
		return 0;
	}
	return (scenegraph::FrameIndex)((timeMillis * fps() + 500) / 1000);
}

int32_t KFAFormat::frameToTime(scenegraph::FrameIndex frame) {
	if (frame <= 0) {
		return 0;
	}
	return (int32_t)((frame * 1000) / fps());
}

bool KFAFormat::parse(const core::String &filename, const io::ArchivePtr &archive, Data &data) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return false;
	}
	uint32_t magic;
	wrap(stream->readUInt32(magic))
	if (magic != FourCC('K', 'w', 'l', 'k')) {
		Log::error("Invalid kwalk magic number");
		return false;
	}
	wrapBool(stream->readPascalStringUInt32LE(data.kv6Name))
	Log::debug("kfa kv6Name: %s", data.kv6Name.c_str());

	uint32_t numHinge;
	wrap(stream->readUInt32(numHinge))
	if (numHinge >= kfa_priv::MAXSPRITES) {
		Log::error("Max allowed hinges exceeded: %u (max is %u)", numHinge, kfa_priv::MAXSPRITES);
		return false;
	}
	data.hinges.clear();
	data.hinges.reserve(numHinge);
	for (uint32_t i = 0; i < numHinge; ++i) {
		Hinge hinge;
		wrap(stream->readInt32(hinge.parent))
		for (int n = 0; n < 2; ++n) {
			wrap(stream->readFloat(hinge.p[n].x))
			wrap(stream->readFloat(hinge.p[n].y))
			wrap(stream->readFloat(hinge.p[n].z))
		}
		for (int n = 0; n < 2; ++n) {
			wrap(stream->readFloat(hinge.v[n].x))
			wrap(stream->readFloat(hinge.v[n].y))
			wrap(stream->readFloat(hinge.v[n].z))
		}
		wrap(stream->readInt16(hinge.vmin))
		wrap(stream->readInt16(hinge.vmax))
		wrap(stream->readInt8(hinge.htype))
		if (stream->skip(7) == -1) {
			Log::error("Could not load kfa file: hinge filler missing");
			return false;
		}
		if (hinge.parent != -1 && (hinge.parent < 0 || (uint32_t)hinge.parent >= numHinge)) {
			Log::error("KFA hinge %u: parent %d out of range", i, hinge.parent);
			return false;
		}
		data.hinges.push_back(hinge);
	}

	wrap(stream->readUInt32(data.numFrames))
	if (data.numFrames > 65536) {
		Log::error("Max allowed frames exceeded: %u", data.numFrames);
		return false;
	}
	if (data.numFrames > 0 && numHinge == 0) {
		Log::error("KFA declares %u frame(s) but zero hinges", data.numFrames);
		return false;
	}
	data.frmval.clear();
	data.frmval.reserve((size_t)data.numFrames * (size_t)numHinge);
	for (uint32_t i = 0; i < data.numFrames; ++i) {
		for (uint32_t j = 0; j < numHinge; ++j) {
			int16_t angle;
			wrap(stream->readInt16(angle))
			data.frmval.push_back(angle);
		}
	}

	uint32_t numSequences;
	wrap(stream->readUInt32(numSequences))
	data.seq.clear();
	data.seq.reserve(numSequences);
	for (uint32_t i = 0; i < numSequences; ++i) {
		Seq seq;
		wrap(stream->readInt32(seq.time))
		wrap(stream->readInt32(seq.frame))
		data.seq.push_back(seq);
	}
	return true;
}

#undef wrap
#undef wrapBool

bool KFAFormat::apply(const Data &data, const voxel::RawVolume *volume, const glm::vec3 &kenPivot,
					  scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette,
					  const core::String &nodePrefix, const core::Buffer<KenSpriteVox> &kenVoxels) {
	if (volume == nullptr) {
		Log::error("KFA apply needs a kv6 volume");
		return false;
	}
	if (data.hinges.empty()) {
		Log::error("KFA has no hinges");
		return false;
	}

	// KWALK floodsucksprite: vis-bit z-slabs, 4-adjacent XY, z-range overlap
	core::Buffer<voxel::RawVolume *> volumes = kfa_priv::splitKenVisSlabs(volume, kenVoxels);
	if (volumes.empty()) {
		Log::warn("KFA vis-slab split produced no limbs - falling back to 18-connected");
		volumes = voxelutil::splitObjects(volume, voxelutil::VisitorOrder::XZmY, voxel::Connectivity::EighteenConnected);
	}
	if (volumes.empty()) {
		Log::error("Could not split kv6 volume into kfa limbs");
		return false;
	}
	Log::debug("KFA split into %i limbs for %i hinges", (int)volumes.size(), (int)data.hinges.size());

	const uint32_t numspr = (uint32_t)volumes.size();
	const uint32_t numhin = (uint32_t)data.hinges.size();
	if (numhin < numspr) {
		Log::warn("KFA has fewer hinges (%u) than split limbs (%u) - extra limbs stay unrigged", numhin, numspr);
	}

	const int fullHeight = volume->region().getHeightInVoxels();
	core::Buffer<glm::vec3> restP;
	core::Buffer<glm::vec3> volumeOrigin;
	restP.reserve(numhin);
	volumeOrigin.reserve(numhin);
	for (uint32_t i = 0; i < numhin; ++i) {
		if (i < numspr) {
			const voxel::Region &r = volumes[i]->region();
			const int kenX0 = r.getLowerX();
			const int kenY0 = r.getLowerZ();
			const int kenZ0 = (fullHeight - 1) - r.getUpperY();
			restP.push_back(glm::vec3((float)kenX0 - kenPivot.x, (float)kenY0 - kenPivot.y, (float)kenZ0 - kenPivot.z));
			// Ken sprite origin in the unsplit KV6 / cropped-limb voxel grid
			volumeOrigin.push_back(glm::vec3((float)kenX0, (float)((fullHeight - 1) - kenZ0), (float)kenY0));
		} else {
			restP.push_back(glm::vec3(0.0f));
			volumeOrigin.push_back(glm::vec3(0.0f));
		}
	}

	Data posed = data;
	for (uint32_t i = 0; i < numhin; ++i) {
		if (posed.hinges[i].parent < 0 && i < numspr) {
			posed.hinges[i].p[0].x = -restP[i].x;
			posed.hinges[i].p[0].y = -restP[i].y;
			posed.hinges[i].p[0].z = -restP[i].z;
		}
	}

	core::Buffer<int32_t> evalOrder;
	if (!kfa_priv::sortHinges(posed.hinges, evalOrder)) {
		kfa_priv::freeVolumes(volumes);
		return false;
	}

	palette::NormalPalette normalPalette;
	normalPalette.slab6();

	core::Buffer<int> nodeIds;
	nodeIds.reserve(numhin);
	for (uint32_t i = 0; i < numhin; ++i) {
		nodeIds.push_back(InvalidNodeId);
	}

	for (int32_t hingeIdx : evalOrder) {
		const Hinge &hinge = posed.hinges[hingeIdx];
		voxel::RawVolume *v = nullptr;
		if ((uint32_t)hingeIdx < numspr) {
			v = volumes[hingeIdx];
			volumes[hingeIdx] = nullptr;
		}

		scenegraph::SceneGraphNode node(v != nullptr ? scenegraph::SceneGraphNodeType::Model
													 : scenegraph::SceneGraphNodeType::Group);
		if (v != nullptr) {
			node.setVolume(v);
			node.setPalette(palette);
			node.setNormalPalette(normalPalette);
			const voxel::Region &r = v->region();
			const glm::vec3 dims(r.getDimensionsInVoxels());
			const glm::vec3 hingeVolume = kfa_priv::hingeVolumePos(volumeOrigin[hingeIdx], kfa_priv::toVec(hinge.p[1]));
			node.setPivot(hingeVolume / dims);
		} else {
			node.setPivot(glm::vec3(0.0f));
		}
		node.setName(core::String::format("%s_hinge_%d%s", nodePrefix.c_str(), hingeIdx, v != nullptr ? "" : "_joint"));
		node.setProperty("kfa.hinge", (uint32_t)hingeIdx);

		int parentNodeId = 0;
		if (hinge.parent >= 0 && (uint32_t)hinge.parent < nodeIds.size() && nodeIds[hinge.parent] != InvalidNodeId) {
			parentNodeId = nodeIds[hinge.parent];
		}
		const int nodeId = sceneGraph.emplace(core::move(node), parentNodeId);
		if (nodeId == InvalidNodeId) {
			Log::error("Failed to add kfa node for hinge %d", hingeIdx);
			kfa_priv::freeVolumes(volumes);
			return false;
		}
		nodeIds[hingeIdx] = nodeId;
	}

	for (uint32_t i = numhin; i < numspr; ++i) {
		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		node.setVolume(volumes[i]);
		volumes[i] = nullptr;
		node.setPalette(palette);
		node.setNormalPalette(normalPalette);
		node.setName(core::String::format("%s_limb_%u", nodePrefix.c_str(), i));
		if (sceneGraph.emplace(core::move(node), 0) == InvalidNodeId) {
			kfa_priv::freeVolumes(volumes);
			return false;
		}
	}
	kfa_priv::freeVolumes(volumes);

	core::Buffer<int32_t> keyTimes;
	core::Buffer<int32_t> keyKenFrames;
	kfa_priv::collectKeyedTimes(posed, keyTimes, keyKenFrames);

	core::Buffer<kfa_priv::KenSprite> sprites;
	core::Buffer<glm::mat4> worldMats;
	core::Buffer<int32_t> angles;
	core::Buffer<scenegraph::FrameIndex> written;
	worldMats.reserve(numhin);

	auto writeFrame = [&](scenegraph::FrameIndex frameIdx, const int32_t *poseAngles) {
		for (scenegraph::FrameIndex already : written) {
			if (already == frameIdx) {
				return;
			}
		}
		written.push_back(frameIdx);

		kfa_priv::evaluatePose(posed, restP, evalOrder, poseAngles, sprites);
		worldMats.clear();
		for (uint32_t i = 0; i < numhin; ++i) {
			worldMats.push_back(kfa_priv::vengiWorldFromSprite(sprites[i], volumeOrigin[i],
															   kfa_priv::toVec(posed.hinges[i].p[1]), i < numspr));
		}

		for (uint32_t i = 0; i < numhin; ++i) {
			if (nodeIds[i] == InvalidNodeId) {
				continue;
			}
			scenegraph::SceneGraphNode &node = sceneGraph.node(nodeIds[i]);
			scenegraph::KeyFrameIndex kfIdx = node.addKeyFrame(frameIdx);
			if (kfIdx == InvalidKeyFrame) {
				kfIdx = node.keyFrameForFrame(frameIdx);
			}
			if (kfIdx == InvalidKeyFrame) {
				continue;
			}
			scenegraph::SceneGraphKeyFrame &kf = node.keyFrame(kfIdx);
			kf.interpolation = scenegraph::InterpolationType::Linear;
			glm::mat4 local = worldMats[i];
			const int32_t parent = posed.hinges[i].parent;
			if (parent >= 0 && (uint32_t)parent < worldMats.size()) {
				local = glm::inverse(worldMats[parent]) * worldMats[i];
			}
			kfa_priv::matrixToLocal(local, kf.transform());
		}
	};

	// Ken interpolates hinge angles between seq poses (animsprite). We are lerping TRS,
	// which flies limbs when pose 1 jumps to pose 3. Bake every vengi frame in those gaps
	// with Ken's wrap-lerp
	for (uint32_t k = 0; k < keyTimes.size(); ++k) {
		const int32_t t0 = keyTimes[k];
		const int32_t f0 = keyKenFrames[k];
		const scenegraph::FrameIndex v0 = timeToFrame(t0);
		if (k + 1 >= keyTimes.size()) {
			kfa_priv::lerpHingeAngles(posed, f0, f0, 0.0f, angles);
			writeFrame(v0, angles.data());
			break;
		}
		const int32_t t1 = keyTimes[k + 1];
		const int32_t f1 = keyKenFrames[k + 1];
		const scenegraph::FrameIndex v1 = timeToFrame(t1);
		if (f0 == f1 || v0 == v1) {
			kfa_priv::lerpHingeAngles(posed, f0, f0, 0.0f, angles);
			writeFrame(v0, angles.data());
			continue;
		}
		const int32_t dt = t1 - t0;
		for (scenegraph::FrameIndex vf = v0; vf <= v1; ++vf) {
			float trat = 0.0f;
			if (vf == v1) {
				trat = 1.0f;
			} else if (vf != v0 && dt != 0) {
				trat = (float)(frameToTime(vf) - t0) / (float)dt;
			}
			kfa_priv::lerpHingeAngles(posed, f0, f1, trat, angles);
			writeFrame(vf, angles.data());
		}
	}
	return true;
}

} // namespace voxelformat
