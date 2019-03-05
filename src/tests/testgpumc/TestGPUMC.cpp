/**
 * @file
 */
#include "TestGPUMC.h"

#include "io/Filesystem.h"
#include "compute/Compute.h"
#include "voxel/MaterialColor.h"
#include "computevideo/ComputeVideo.h"
#include "voxel/polyvox/Region.h"
#include "voxel/polyvox/Voxel.h"
#include "voxel/polyvox/RawVolumeWrapper.h"
#include "voxel/polyvox/VolumeVisitor.h"
#include "voxel/generator/NoiseGenerator.h"

#define GL_INTEROP 0

namespace {
	// may not be smaller than 64
	const int REGION_SIZE = 64;
	const int isolevel = 51;
}

TestGPUMC::TestGPUMC(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		Super(metric, filesystem, eventBus, timeProvider),
		_computeShader(compute::MarchingcubesShader::getInstance()),
		_computeShaderBuffer(compute::MarchingcubesBufferShader::getInstance()),
		_renderShader(shader::VertexShader::getInstance()) {
	init(ORGANISATION, "testgpumc");
}

core::AppState TestGPUMC::onConstruct() {
	core::AppState state = Super::onConstruct();
	core::Var::get("use3dtextures", "false");
	return state;
}

core::AppState TestGPUMC::onInit() {
	core::AppState state = Super::onInit();
	if (state != core::AppState::Running) {
		return state;
	}

	if (!computevideo::init()) {
		Log::error("Failed to init videocompute context");
		return core::AppState::InitFailure;
	}

	if (!compute::init()) {
		Log::error("Failed to init compute context");
		return core::AppState::InitFailure;
	}

	if (!compute::hasFeature(compute::Feature::VideoSharing)) {
		Log::error("The compute context needs the video state sharing feature");
		return core::AppState::InitFailure;
	}

	_writingTo3DTextures = core::Var::getSafe("use3dtextures")->boolVal();
	if (_writingTo3DTextures) {
		if (!compute::hasFeature(compute::Feature::Write3dTextures)) {
			Log::warn("The compute context is not able to write to 3d features");
			_writingTo3DTextures = false;
		}
	}
	if (_writingTo3DTextures) {
		Log::info("write to 3d textures");
	} else {
		Log::info("write to buffers");
	}

	if (!voxel::initDefaultMaterialColors()) {
		Log::error("Failed to initialize the palette data");
		return core::AppState::InitFailure;
	}

	voxel::Region region(0, 0, 0, REGION_SIZE - 1, REGION_SIZE - 1, REGION_SIZE - 1);
	_volume = std::make_shared<voxel::RawVolume>(region);
	math::Random random;
	voxel::RawVolumeWrapper wrapper(_volume.get());
	voxel::noisegen::generate(wrapper, 4, 2.0, 0.01, 0.5, voxel::noisegen::NoiseType::ridgedMF, random);
	const int amount = voxel::visitVolume(*_volume.get(), [] (int x, int y, int z, const voxel::Voxel& voxel) {
	});
	Log::info("%i voxels", amount);

	if (!_renderShader.setup()) {
		Log::error("Failed to setup render shader");
		return core::AppState::InitFailure;
	}

	_computeShader.addDefine("SIZE", std::to_string(REGION_SIZE));
	if (!_computeShader.setup()) {
		Log::error("Failed to init compute shader for using 3d textures");
		if (_writingTo3DTextures) {
			return core::AppState::InitFailure;
		}
	}
	_computeShaderBuffer.addDefine("SIZE", std::to_string(REGION_SIZE));
	if (!_computeShaderBuffer.setup()) {
		Log::error("Failed to init compute shader for using buffer");
		if (!_writingTo3DTextures) {
			return core::AppState::InitFailure;
		}
	}

	if (_writingTo3DTextures) {
		// Create images for the HistogramPyramid
		int bufferSize = REGION_SIZE;
		compute::TextureConfig cfg;
		cfg.dataformat(compute::TextureDataFormat::UNSIGNED_INT8).format(compute::TextureFormat::RGBA).type(compute::TextureType::Texture3D);

		_images.push_back(std::make_shared<compute::Texture>(cfg, glm::ivec3(bufferSize)));
		Log::info("Texture %i size is at %i", int(_images.size()), bufferSize);
		bufferSize /= 2;

		_images.push_back(std::make_shared<compute::Texture>(cfg, glm::ivec3(bufferSize)));
		Log::info("Texture %i size is at %i", int(_images.size()), bufferSize);
		bufferSize /= 2;

		cfg.dataformat(compute::TextureDataFormat::UNSIGNED_INT16).format(compute::TextureFormat::R);
		_images.push_back(std::make_shared<compute::Texture>(cfg, glm::ivec3(bufferSize)));
		Log::info("Texture %i size is at %i", int(_images.size()), bufferSize);
		bufferSize /= 2;

		_images.push_back(std::make_shared<compute::Texture>(cfg, glm::ivec3(bufferSize)));
		Log::info("Texture %i size is at %i", int(_images.size()), bufferSize);
		bufferSize /= 2;

		_images.push_back(std::make_shared<compute::Texture>(cfg, glm::ivec3(bufferSize)));
		Log::info("Texture %i size is at %i", int(_images.size()), bufferSize);
		bufferSize /= 2;

		cfg.dataformat(compute::TextureDataFormat::UNSIGNED_INT32);
		const int maxBuffers = log2((float) REGION_SIZE);
		Log::info("Max buffers: %i", maxBuffers);
		for (int i = 5; i < maxBuffers; i++) {
			if (bufferSize == 1) {
				Log::info("Ensure that the texture size is not 1x1x1");
				bufferSize = 2;
			}
			_images.push_back(std::make_shared<compute::Texture>(cfg, glm::ivec3(bufferSize)));
			Log::info("Texture %i size is at %i", int(_images.size()), bufferSize);
			bufferSize /= 2;
		}

		for (auto& image : _images) {
			if (!image->upload(nullptr)) {
				Log::error("Failed to upload the compute texture for the histogram pyramid");
				return core::AppState::InitFailure;
			}
		}
	} else {
		int bufferSize = REGION_SIZE * REGION_SIZE * REGION_SIZE;
		buffers.push_back(compute::createBuffer(compute::BufferFlag::ReadWrite, sizeof(char) * bufferSize));
		bufferSize /= 8;
		buffers.push_back(compute::createBuffer(compute::BufferFlag::ReadWrite, sizeof(char) * bufferSize));
		bufferSize /= 8;
		buffers.push_back(compute::createBuffer(compute::BufferFlag::ReadWrite, sizeof(short) * bufferSize));
		bufferSize /= 8;
		buffers.push_back(compute::createBuffer(compute::BufferFlag::ReadWrite, sizeof(short) * bufferSize));
		bufferSize /= 8;
		buffers.push_back(compute::createBuffer(compute::BufferFlag::ReadWrite, sizeof(short) * bufferSize));
		bufferSize /= 8;
		for (int i = 5; i < log2((float) REGION_SIZE); i++) {
			buffers.push_back(compute::createBuffer(compute::BufferFlag::ReadWrite, sizeof(int) * bufferSize));
			bufferSize /= 8;
		}

		cubeIndexesBuffer = compute::createBuffer(compute::BufferFlag::WriteOnly, sizeof(char) * REGION_SIZE * REGION_SIZE * REGION_SIZE);
		if (cubeIndexesBuffer == compute::InvalidId) {
			Log::error("Failed to create the cube indexes compute buffer");
			return core::AppState::InitFailure;
		}
		compute::TextureConfig textureCfg;
		textureCfg.dataformat(compute::TextureDataFormat::UNSIGNED_INT8);
		textureCfg.type(compute::TextureType::Texture3D);
		textureCfg.format(compute::TextureFormat::R);
		cubeIndexesImage = std::make_shared<compute::Texture>(textureCfg, glm::ivec3(REGION_SIZE)); // TODO: this is CL_MEM_READ_ONLY,
		if (!cubeIndexesImage->upload(nullptr)) {
			Log::error("Failed to upload the cube indexes data");
			return core::AppState::InitFailure;
		}
	}

	compute::TextureConfig inputCfg;
	inputCfg.dataformat(compute::TextureDataFormat::UNSIGNED_INT8).format(compute::TextureFormat::R).type(compute::TextureType::Texture3D);
	_rawData = std::make_shared<compute::Texture>(inputCfg, glm::ivec3(REGION_SIZE));
	if (!_rawData->upload(_volume->data())) {
		Log::error("Failed to upload the volume data");
		return core::AppState::InitFailure;
	}

	_vboIdx = _vbo.create();

	struct V {
		glm::vec3 pos;
		glm::vec3 norm;
	};
	static_assert(sizeof(V) == 24, "Padding/Alignment doesn't match requirements");
	_vbo.addAttribute(_renderShader.getPosAttribute(_vboIdx, &V::pos));
	_vbo.addAttribute(_renderShader.getNormAttribute(_vboIdx, &V::norm));
	_vbo.setMode(_vboIdx, video::BufferMode::Static);

	return state;
}

core::AppState TestGPUMC::onCleanup() {
	core::AppState state = Super::onCleanup();
	if (_rawData) {
		_rawData->shutdown();
	}
	for (auto& i : _images) {
		i->shutdown();
	}
	for (auto& i : buffers) {
		compute::deleteBuffer(i);
	}
	_images.clear();
	_vbo.shutdown();
	_renderShader.shutdown();
	_computeShader.shutdown();
	_computeShaderBuffer.shutdown();
	if (cubeIndexesImage) {
		cubeIndexesImage->shutdown();
	}
	compute::deleteBuffer(_vboComputeBufferId);
	compute::deleteBuffer(cubeIndexesBuffer);
	computevideo::shutdown();
	compute::shutdown();
	return state;
}

int TestGPUMC::calculateTotalSum() {
	int sum[8] = {0};
	if (_writingTo3DTextures) {
		_computeShader.classifyCubes(*_images[0], *_rawData, isolevel, glm::ivec3(REGION_SIZE));
		// Run base to first level
		int previous = REGION_SIZE / 2;
		const int maxBuffers = log2((float) REGION_SIZE) - 1;
		for (int i = 0; i < maxBuffers; i++) {
			Log::info("Texture %i and %i (%i maxBuffers, %i images) construct histogram pyramid %i", i, i + 1, maxBuffers, (int)_images.size(), previous);
			_computeShader.constructHPLevel(*_images[i], *_images[i + 1], glm::ivec3(previous));
			previous /= 2;
		}
		if (!compute::readTexture(*_images[_images.size() - 1], sum, glm::ivec3(0), glm::ivec3(2), true)) {
			Log::warn("Couldn't read sum from texture.");
			return -1;
		}
	} else {
		_computeShaderBuffer.classifyCubes(buffers[0], cubeIndexesBuffer, *_rawData, isolevel, glm::ivec3(REGION_SIZE));
		glm::ivec3 offset(0);
		glm::ivec3 region(REGION_SIZE);
		Log::info("Copy the buffer back into the image");
		compute::copyBufferToImage(cubeIndexesBuffer, cubeIndexesImage->handle(), 0, offset, region);
		// Run base to first level
		Log::info("Construct the different histogram pyramids");
		_computeShaderBuffer.constructHPLevelCharChar(buffers[0], buffers[1], glm::ivec3(REGION_SIZE / 2));
		_computeShaderBuffer.constructHPLevelCharShort(buffers[1], buffers[2], glm::ivec3(REGION_SIZE / 4));
		_computeShaderBuffer.constructHPLevelShortShort(buffers[2], buffers[3], glm::ivec3(REGION_SIZE / 8));
		_computeShaderBuffer.constructHPLevelShortShort(buffers[3], buffers[4], glm::ivec3(REGION_SIZE / 16));
		_computeShaderBuffer.constructHPLevelShortInt(buffers[4], buffers[5], glm::ivec3(REGION_SIZE / 32));

		int previous = REGION_SIZE / 64;
		// Run level 2 to top level
		for (int i = 5; i < log2(REGION_SIZE) - 1; i++) {
			previous /= 2; // TODO: correct? shouldn't this be after the kernel was executed.
			_computeShaderBuffer.constructHPLevel(buffers[i], buffers[i + 1], glm::ivec3(previous));
		}
		if (!compute::readBuffer(buffers[buffers.size() - 1], sizeof(sum), sum)) {
			Log::warn("Couldn't read sum from texture.");
			return -1;
		}
		compute::finish();
	}

	Log::info("Constructed histogram pyramids - reading back the sum");
	return sum[0] + sum[1] + sum[2] + sum[3] + sum[4] + sum[5] + sum[6] + sum[7];
}

void TestGPUMC::extractSurfaces() {
	if (!_extractSurface) {
		return;
	}
	_totalSum = calculateTotalSum();
	if (_totalSum <= 0) {
		Log::warn("No triangles were extracted. Check isovalue.");
		return;
	}
	_extractSurface = false;

	Log::info("Prepare the vbo.");
	const size_t vboSize = _totalSum * 18 * sizeof(float);
	_vbo.update(_vboIdx, nullptr, vboSize);

	if (!_writingTo3DTextures && _vboComputeBufferId == compute::InvalidId) {
		_vboComputeBufferId = computevideo::createBuffer(compute::BufferFlag::ReadWrite, _vbo, _vboIdx);
		if (_vboComputeBufferId == compute::InvalidId) {
			Log::error("Failed to generate the vbo compute buffer");
			return;
		}
	}

	// Increase the globalWorkSize so that it is divideable by 64
	int globalWorkSize = _totalSum + 64 - (_totalSum - 64 * (_totalSum / 64));

	if (_writingTo3DTextures) {
		_computeShader.traverseHP(*_images[0], *_images[1], *_images[2], *_images[3], *_images[4], *_images[5],
				_vbo, isolevel, _totalSum, glm::ivec3(globalWorkSize));
		return;
	}

	compute::Texture& raw = *_rawData;
	compute::Texture& indices = *cubeIndexesImage;
	_computeShaderBuffer.traverseHP(raw, indices, buffers[0], buffers[1], buffers[2], buffers[3], buffers[4], buffers[5],
			_vboComputeBufferId, isolevel, _totalSum, glm::ivec3(globalWorkSize));
}

void TestGPUMC::doRender() {
	extractSurfaces();
	video::ScopedShader scoped(_renderShader);
	video::ScopedBuffer scopedBuf(_vbo);
	_renderShader.setColor(core::Color::Green);
	video::drawArrays(video::Primitive::Triangles, _totalSum * 3);
}

TEST_APP(TestGPUMC)
