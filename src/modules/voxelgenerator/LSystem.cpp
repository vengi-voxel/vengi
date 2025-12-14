/**
 * @file
 */

#include "LSystem.h"
#include "app/I18N.h"
#include "core/Tokenizer.h"
#include "core/Log.h"

namespace voxelgenerator {
namespace lsystem {

const core::DynamicArray<LSystemCommand>& getLSystemCommands() {
	static const core::DynamicArray<LSystemCommand> commands{
		{'F', N_("Draw line forwards")},
		{'(', N_("Set voxel type")},
		{'b', N_("Move backwards (no drawing)")},
		{'L', N_("Leaf")},
		{'+', N_("Rotate right")},
		{'-', N_("Rotate left")},
		{'>', N_("Rotate forward")},
		{'<', N_("Rotate back")},
		{'#', N_("Increment width")},
		{'!', N_("Decrement width")},
		{'[', N_("Push")},
		{']', N_("Pop")}
	};
	return commands;
}

void prepareState(const LSystemConfig &conf, LSystemState &state) {
	state.sentence = conf.axiom;
	state.position = conf.position;
	state.angle = conf.angle;
	state.length = conf.length;
	state.width = conf.width;
	state.widthIncrement = conf.widthIncrement;
	state.leafRadius = conf.leafRadius;

	for (int i = 0; i < conf.iterations; i++) {
		core::String nextSentence = "";

		for (size_t j = 0; j < state.sentence.size(); ++j) {
			const char current = state.sentence[j];
			bool found = false;
			for (const auto &rule : conf.rules) {
				if (rule.a == current) {
					found = true;
					nextSentence += rule.b;
					break;
				}
			}
			if (!found) {
				nextSentence += current;
			}
			if (nextSentence.size() > 1024 * 1024) {
				Log::warn("LSystem sentence length exceeded limit");
				state.sentence = nextSentence;
				return;
			}
		}

		state.sentence = nextSentence;
	}
}

// https://paulbourke.net/fractals/lsys/
core::DynamicArray<LSystemTemplate> defaultTemplates() {
	core::DynamicArray<LSystemTemplate> templates;

	// Simple Branching Tree
	LSystemTemplate simpleTree;
	simpleTree.name = "Simple Tree";
	simpleTree.description = "A basic tree structure that branches in 4 directions";
	simpleTree.config.axiom = "X";
	simpleTree.config.angle = glm::radians(25.0f);
	simpleTree.config.length = 2.0f;
	simpleTree.config.width = 2.0f;
	simpleTree.config.widthIncrement = 0.4f;
	simpleTree.config.iterations = 4;
	simpleTree.config.leafRadius = 4.0f;
	simpleTree.config.rules = {
		{ 'X', "F[+X][-X][>X][<X]LX" },
		{ 'F', "FF" }
	};
	templates.push_back(simpleTree);

	// Dense Bush
	LSystemTemplate bush;
	bush.name = "Dense Bush";
	bush.description = "Short, wide, and bushy with many leaves";
	bush.config.axiom = "F";
	bush.config.angle = glm::radians(22.5f);
	bush.config.length = 1.5f;
	bush.config.width = 1.0f;
	bush.config.widthIncrement = 0.2f;
	bush.config.iterations = 3;
	bush.config.leafRadius = 5.0f;
	bush.config.rules = {
		{ 'F', "FF[+F][-F][>F][<F]L" }
	};
	templates.push_back(bush);

	// Alien Spiral
	LSystemTemplate alienSpiral;
	alienSpiral.name = "Alien Spiral";
	alienSpiral.description = "Asymmetric growth using only two rotation axes";
	alienSpiral.config.axiom = "X";
	alienSpiral.config.angle = glm::radians(30.0f);
	alienSpiral.config.length = 2.0f;
	alienSpiral.config.width = 1.5f;
	alienSpiral.config.widthIncrement = 0.1f;
	alienSpiral.config.iterations = 5;
	alienSpiral.config.leafRadius = 4.0f;
	alienSpiral.config.rules = {
		{ 'X', "F[+X][>X]LX" },
		{ 'F', "F" }
	};
	templates.push_back(alienSpiral);

	// Geometric Structure
	LSystemTemplate geometric;
	geometric.name = "Geometric";
	geometric.description = "90-degree branching for a synthetic look";
	geometric.config.axiom = "F";
	geometric.config.angle = glm::radians(90.0f);
	geometric.config.length = 3.0f;
	geometric.config.width = 1.0f;
	geometric.config.widthIncrement = 0.0f;
	geometric.config.iterations = 3;
	geometric.config.leafRadius = 2.0f;
	geometric.config.rules = {
		{ 'F', "F[+F]F[>F]F" }
	};
	templates.push_back(geometric);

	LSystemTemplate sympodialTree;
	sympodialTree.name = "Sympodial Tree";
	sympodialTree.description = "A tree growth pattern where the main stem terminates and growth continues from lateral branches.";
	sympodialTree.config.axiom = "F";
	sympodialTree.config.angle = glm::radians(25.0f);
	sympodialTree.config.length = 2.0f;
	sympodialTree.config.width = 2.0f;
	sympodialTree.config.widthIncrement = 0.3f;
	sympodialTree.config.iterations = 4;
	sympodialTree.config.leafRadius = 4.0f;
	sympodialTree.config.rules = {
		{ 'F', "F[+F][>F][<F]L" }
	};
	templates.push_back(sympodialTree);

	LSystemTemplate monopodialTree;
	monopodialTree.name = "Monopodial Tree";
	monopodialTree.description = "A tree growth pattern with a single main trunk that continues to grow.";
	monopodialTree.config.axiom = "X";
	monopodialTree.config.angle = glm::radians(20.0f);
	monopodialTree.config.length = 2.0f;
	monopodialTree.config.width = 2.0f;
	monopodialTree.config.widthIncrement = 0.3f;
	monopodialTree.config.iterations = 4;
	monopodialTree.config.leafRadius = 4.0f;
	monopodialTree.config.rules = {
		{ 'X', "F[+X][-X][>X][<X]LX" },
		{ 'F', "FF" }
	};
	templates.push_back(monopodialTree);

	LSystemTemplate tallShrub;
	tallShrub.name = "Tall Shrub";
	tallShrub.description = "A tall, thin shrub with upward reaching branches.";
	tallShrub.config.axiom = "F";
	tallShrub.config.angle = glm::radians(22.5f);
	tallShrub.config.length = 1.5f;
	tallShrub.config.width = 1.0f;
	tallShrub.config.widthIncrement = 0.2f;
	tallShrub.config.iterations = 4;
	tallShrub.config.leafRadius = 3.0f;
	tallShrub.config.rules = {
		{ 'F', "FF[+F][-F][>F][<F]L" }
	};
	templates.push_back(tallShrub);

	LSystemTemplate broadCanopy;
	broadCanopy.name = "Broad Canopy";
	broadCanopy.description = "A tree with a wide, spreading canopy.";
	broadCanopy.config.axiom = "X";
	broadCanopy.config.angle = glm::radians(30.0f);
	broadCanopy.config.length = 2.0f;
	broadCanopy.config.width = 2.0f;
	broadCanopy.config.widthIncrement = 0.3f;
	broadCanopy.config.iterations = 4;
	broadCanopy.config.leafRadius = 4.0f;
	broadCanopy.config.rules = {
		{ 'X', "F[+X][-X][>X][<X]LX" },
		{ 'F', "F" }
	};
	templates.push_back(broadCanopy);

	LSystemTemplate coniferLike;
	coniferLike.name = "Conifer-like";
	coniferLike.description = "A cone-shaped tree resembling a conifer.";
	coniferLike.config.axiom = "X";
	coniferLike.config.angle = glm::radians(20.0f);
	coniferLike.config.length = 2.0f;
	coniferLike.config.width = 2.0f;
	coniferLike.config.widthIncrement = 0.3f;
	coniferLike.config.iterations = 4;
	coniferLike.config.leafRadius = 4.0f;
	coniferLike.config.rules = {
		{ 'X', "F[+X][-X][>X][<X]FX" },
		{ 'F', "FF" }
	};
	templates.push_back(coniferLike);

	LSystemTemplate twistedWeed;
	twistedWeed.name = "Twisted Weed";
	twistedWeed.description = "A chaotic, twisting weed-like structure.";
	twistedWeed.config.axiom = "F";
	twistedWeed.config.angle = glm::radians(35.0f);
	twistedWeed.config.length = 1.5f;
	twistedWeed.config.width = 1.0f;
	twistedWeed.config.widthIncrement = 0.1f;
	twistedWeed.config.iterations = 4;
	twistedWeed.config.leafRadius = 3.0f;
	twistedWeed.config.rules = {
		{ 'F', "F[+F]F[-F][>F]L" }
	};
	templates.push_back(twistedWeed);

	LSystemTemplate fern3D;
	fern3D.name = "Fern 3D";
	fern3D.description = "A fern-like plant with fronds.";
	fern3D.config.axiom = "X";
	fern3D.config.angle = glm::radians(25.0f);
	fern3D.config.length = 1.5f;
	fern3D.config.width = 1.0f;
	fern3D.config.widthIncrement = 0.1f;
	fern3D.config.iterations = 4;
	fern3D.config.leafRadius = 3.0f;
	fern3D.config.rules = {
		{ 'X', "F[+X][-X]LX" },
		{ 'F', "FF" }
	};
	templates.push_back(fern3D);

	LSystemTemplate coralReef;
	coralReef.name = "Coral Reef";
	coralReef.description = "A branching structure resembling coral.";
	coralReef.config.axiom = "F";
	coralReef.config.angle = glm::radians(30.0f);
	coralReef.config.length = 1.5f;
	coralReef.config.width = 1.5f;
	coralReef.config.widthIncrement = 0.1f;
	coralReef.config.iterations = 3;
	coralReef.config.leafRadius = 2.0f;
	coralReef.config.rules = {
		{ 'F', "F[+F]F[-F][>F][<F]L" }
	};
	templates.push_back(coralReef);

	LSystemTemplate alienFlower;
	alienFlower.name = "Alien Flower";
	alienFlower.description = "An exotic, alien-looking flower structure.";
	alienFlower.config.axiom = "X";
	alienFlower.config.angle = glm::radians(45.0f);
	alienFlower.config.length = 2.0f;
	alienFlower.config.width = 1.0f;
	alienFlower.config.widthIncrement = 0.1f;
	alienFlower.config.iterations = 4;
	alienFlower.config.leafRadius = 3.0f;
	alienFlower.config.rules = {
		{ 'X', "F[+X][-X][>X][<X]LX" },
		{ 'F', "F" }
	};
	templates.push_back(alienFlower);

	LSystemTemplate bamboo;
	bamboo.name = "Bamboo";
	bamboo.description = "Tall, straight segments with leaves at joints.";
	bamboo.config.axiom = "F";
	bamboo.config.angle = glm::radians(15.0f);
	bamboo.config.length = 3.0f;
	bamboo.config.width = 1.5f;
	bamboo.config.widthIncrement = 0.0f;
	bamboo.config.iterations = 4;
	bamboo.config.leafRadius = 4.0f;
	bamboo.config.rules = {
		{ 'F', "FF[+L][-L][>L][<L]F" }
	};
	templates.push_back(bamboo);

	LSystemTemplate pineTree;
	pineTree.name = "Pine Tree";
	pineTree.description = "A tall, conical evergreen tree.";
	pineTree.config.axiom = "X";
	pineTree.config.angle = glm::radians(18.0f);
	pineTree.config.length = 2.0f;
	pineTree.config.width = 2.0f;
	pineTree.config.widthIncrement = 0.2f;
	pineTree.config.iterations = 5;
	pineTree.config.leafRadius = 4.0f;
	pineTree.config.rules = {
		{ 'X', "F[+X][-X][>X][<X]FX" },
		{ 'F', "FF" }
	};
	templates.push_back(pineTree);

	// https://en.wikipedia.org/wiki/Dragon_curve
	LSystemTemplate dragonCurve;
	dragonCurve.name = "Dragon Curve";
	dragonCurve.description = "The famous fractal dragon curve.";
	dragonCurve.config.axiom = "FX";
	dragonCurve.config.angle = glm::radians(90.0f);
	dragonCurve.config.length = 2.0f;
	dragonCurve.config.width = 1.0f;
	dragonCurve.config.widthIncrement = 0.0f;
	dragonCurve.config.iterations = 10;
	dragonCurve.config.leafRadius = 2.0f;
	dragonCurve.config.rules = {
		{ 'X', "X+YF+" },
		{ 'Y', "-FX-Y" }
	};
	templates.push_back(dragonCurve);

	// https://en.wikipedia.org/wiki/Sierpi%C5%84ski_curve
	LSystemTemplate sierpinski;
	sierpinski.name = "Sierpinski Triangle";
	sierpinski.description = "Sierpinski arrowhead curve.";
	sierpinski.config.axiom = "XF";
	sierpinski.config.angle = glm::radians(60.0f);
	sierpinski.config.length = 2.0f;
	sierpinski.config.width = 1.0f;
	sierpinski.config.widthIncrement = 0.0f;
	sierpinski.config.iterations = 6;
	sierpinski.config.leafRadius = 2.0f;
	sierpinski.config.rules = {
		{ 'X', "YF+XF+Y" },
		{ 'Y', "XF-YF-X" }
	};
	templates.push_back(sierpinski);

	// https://en.wikipedia.org/wiki/Hilbert_curve
	LSystemTemplate hilbert;
	hilbert.name = "Hilbert Curve";
	hilbert.description = "Space-filling Hilbert curve.";
	hilbert.config.axiom = "X";
	hilbert.config.angle = glm::radians(90.0f);
	hilbert.config.length = 2.0f;
	hilbert.config.width = 1.0f;
	hilbert.config.widthIncrement = 0.0f;
	hilbert.config.iterations = 5;
	hilbert.config.leafRadius = 2.0f;
	hilbert.config.rules = {
		{ 'X', "-YF+XFX+FY-" },
		{ 'Y', "+XF-YFY-FX+" }
	};
	templates.push_back(hilbert);

	LSystemTemplate weepingWillow;
	weepingWillow.name = "Weeping Willow";
	weepingWillow.description = "A tree with drooping branches.";
	weepingWillow.config.axiom = "F";
	weepingWillow.config.angle = glm::radians(25.0f);
	weepingWillow.config.length = 2.0f;
	weepingWillow.config.width = 2.0f;
	weepingWillow.config.widthIncrement = 0.2f;
	weepingWillow.config.iterations = 4;
	weepingWillow.config.leafRadius = 4.0f;
	weepingWillow.config.rules = {
		{ 'F', "FF[>>F][<<F][+F][-F]" }
	};
	templates.push_back(weepingWillow);

	LSystemTemplate cactus;
	cactus.name = "Cactus";
	cactus.description = "A simple cactus structure.";
	cactus.config.axiom = "F";
	cactus.config.angle = glm::radians(25.0f);
	cactus.config.length = 3.0f;
	cactus.config.width = 3.0f;
	cactus.config.widthIncrement = -0.2f;
	cactus.config.iterations = 3;
	cactus.config.leafRadius = 2.0f;
	cactus.config.rules = {
		{ 'F', "F[+F]F[-F]F" }
	};
	templates.push_back(cactus);

	return templates;
}

bool parseRules(const core::String& rulesStr, core::DynamicArray<Rule>& rules) {
	core::Tokenizer tokenizer(rulesStr, " \n");
	while (tokenizer.hasNext()) {
		const core::String block = tokenizer.next();
		if (block != "{") {
			Log::error("Expected '{', but got %s", block.c_str());
			return false;
		}
		while (tokenizer.hasNext()) {
			const core::String a = tokenizer.next();
			if (a == "}") {
				break;
			}
			if (a.size() != 1) {
				Log::error("Expected single char, but got '%s'", a.c_str());
				return false;
			}
			if (!tokenizer.hasNext()) {
				Log::error("Expected lsystem rule string");
				return false;
			}
			Rule rule;
			rule.a = a[0];
			rule.b = tokenizer.next();
			if (rule.b == "}") {
				Log::error("Expected lsystem rule string, but got '}'");
				return false;
			}
			rules.push_back(rule);
		}
	}
	return true;
}

}
}
