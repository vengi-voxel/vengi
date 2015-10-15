#include "NoisePPNoise.h"
#include <Noise.h>
#include <NoiseUtils.h>
#include <iostream>

namespace noise {

void NoisePPNoise::init() {
	noisepp::ConstantModule lowConstant;
	lowConstant.setValue(0);

	noisepp::ConstantModule highConstant;
	highConstant.setValue(1);

	noisepp::BillowModule plainNoise;
	plainNoise.setOctaveCount(1);
	plainNoise.setQuality(noisepp::NOISE_QUALITY_STD);
	plainNoise.setFrequency(0.25);
	plainNoise.setLacunarity(2.0);
	plainNoise.setPersistence(1);

	noisepp::PerlinModule highlandNoise;
	highlandNoise.setOctaveCount(2);
	highlandNoise.setQuality(noisepp::NOISE_QUALITY_STD);
	highlandNoise.setFrequency(0.5);
	highlandNoise.setLacunarity(2.0);
	highlandNoise.setPersistence(1);

	noisepp::RidgedMultiModule mountainNoise;
	mountainNoise.setOctaveCount(4);
	mountainNoise.setQuality(noisepp::NOISE_QUALITY_STD);
	mountainNoise.setFrequency(0.8);
	mountainNoise.setLacunarity(2.0);

	noisepp::PerlinModule controlNoise;
	controlNoise.setOctaveCount(1);
	controlNoise.setQuality(noisepp::NOISE_QUALITY_STD);
	controlNoise.setFrequency(0.05);
	controlNoise.setLacunarity(2.0);
	controlNoise.setPersistence(1);

	noisepp::SelectModule highlandMountainSelection;
	highlandMountainSelection.setControlModule(controlNoise);
	highlandMountainSelection.setSourceModule(0, highlandNoise);
	highlandMountainSelection.setSourceModule(1, mountainNoise);
	highlandMountainSelection.setLowerBound(0.2);

	noisepp::SelectModule plainHighlandSelection;
	plainHighlandSelection.setControlModule(controlNoise);
	plainHighlandSelection.setSourceModule(0, plainNoise);
	plainHighlandSelection.setSourceModule(1, highlandMountainSelection);
	plainHighlandSelection.setLowerBound(0.0);

	_buffer = new noisepp::Real[_w * _h];
	// create a builder
	noisepp::utils::PlaneBuilder2D builder;
	// set the source module
	builder.setModule(plainHighlandSelection);
	// set the buffer
	builder.setDestination(_buffer);
	// set the buffer size
	builder.setSize(_w, _h);
	// set the plane bounds - from (0.5|0) to (1.5|1)
	builder.setBounds(0.5, 0, 1.5, 1);
	// build
	builder.build();

	// create an image
	noisepp::utils::Image img;
	img.create(_w, _h);
	// create the renderer and add some gradients to create a heat-vision like material
	noisepp::utils::GradientRenderer renderer;
	// renderer.addGradient (<value>, noisepp::utils::ColourValue(<red>, <green>, <blue>));
	renderer.addGradient(-1.0, noisepp::utils::ColourValue(0.0f, 0.0f, 0.2f));
	renderer.addGradient(-0.8, noisepp::utils::ColourValue(0.0f, 0.0f, 0.6f));
	renderer.addGradient(0.0, noisepp::utils::ColourValue(1.0f, 0.0f, 0.0f));
	renderer.addGradient(0.6, noisepp::utils::ColourValue(1.0f, 1.0f, 0.0f));
	renderer.addGradient(1.0, noisepp::utils::ColourValue(1.0f, 1.0f, 1.0f));
	// render the image
	renderer.renderImage(img, _buffer);
	// save the image to an BMP
	img.saveBMP("output.bmp");

}

double NoisePPNoise::get(double x, double y, double z, double worldDimension) {
	int value = x + y * _h;
	return _buffer[value];
}

}
