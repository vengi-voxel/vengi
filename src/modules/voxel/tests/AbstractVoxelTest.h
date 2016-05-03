#include "core/tests/AbstractTest.h"
#include "voxel/polyvox/PagedVolume.h"
#include "voxel/Voxel.h"

namespace voxel {

class AbstractVoxelTest: public core::AbstractTest {
protected:
	class Pager: public PagedVolume::Pager {
	public:
		bool pageIn(const Region& region, PagedVolume::Chunk* chunk) override {
			const glm::ivec3 center(region.getWidthInVoxels() / 2, region.getHeightInVoxels() / 2, region.getDepthInVoxels() / 2);
			for (int z = 0; z < region.getDepthInVoxels(); ++z) {
				for (int y = 0; y < region.getHeightInVoxels(); ++y) {
					for (int x = 0; x < region.getWidthInVoxels(); ++x) {
						const glm::ivec3 pos(x, y, z);
						const int distance = (pos - center).length();
						Voxel uVoxelValue = createVoxel(Air);
						if (distance <= 30) {
							uVoxelValue = createVoxel(Grass1);
						}

						chunk->setVoxel(x, y, z, uVoxelValue);
					}
				}
			}
			return true;
		}

		void pageOut(const Region& region, PagedVolume::Chunk* chunk) override {
		}
	};
};

}
