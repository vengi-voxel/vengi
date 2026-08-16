/**
 * @file
 */

#include "core/ProgressBar.h"
#include "core/IProgress.h"
#include "core/ProgressScope.h"
#include "core/SharedProgress.h"
#include <gtest/gtest.h>

namespace core {

class ProgressBarTest : public testing::Test {};

TEST_F(ProgressBarTest, testFormatEmptyAndFull) {
	char buf[128];
	ASSERT_GT(ProgressBar::format(buf, sizeof(buf), "load", 0.0f, 10), 0);
	EXPECT_STREQ(buf, "[----------]   0% load");

	ASSERT_GT(ProgressBar::format(buf, sizeof(buf), "load", 1.0f, 10), 0);
	EXPECT_STREQ(buf, "[##########] 100% load");
}

TEST_F(ProgressBarTest, testFormatPartial) {
	char buf[128];
	ASSERT_GT(ProgressBar::format(buf, sizeof(buf), "save", 0.25f, 8), 0);
	EXPECT_STREQ(buf, "[##------]  25% save");
}

TEST_F(ProgressBarTest, testFormatClampsAndRejects) {
	char buf[128];
	ASSERT_GT(ProgressBar::format(buf, sizeof(buf), "x", 2.0f, 4), 0);
	EXPECT_STREQ(buf, "[####] 100% x");

	EXPECT_EQ(-1, ProgressBar::format(nullptr, 10, "x", 1.0f, 4));
	EXPECT_EQ(-1, ProgressBar::format(buf, 4, "toolongname", 1.0f, 20));
}

TEST_F(ProgressBarTest, testMode) {
	const ProgressBar::Mode previous = ProgressBar::mode();
	ProgressBar::setMode(ProgressBar::Mode::Never);
	EXPECT_EQ(ProgressBar::Mode::Never, ProgressBar::mode());
	ProgressBar::setMode(ProgressBar::Mode::Always);
	EXPECT_EQ(ProgressBar::Mode::Always, ProgressBar::mode());
	ProgressBar::setMode(ProgressBar::Mode::Auto);
	EXPECT_EQ(ProgressBar::Mode::Auto, ProgressBar::mode());
	ProgressBar::setMode(previous);
}

class RecordingProgress : public IProgress {
public:
	float last = -1.0f;
	char text[64] = "";

	void setProgress(float value) override {
		last = value;
	}

	void setText(const char *t) override {
		if (t == nullptr) {
			text[0] = '\0';
			return;
		}
		size_t i = 0;
		for (; t[i] != '\0' && i + 1 < sizeof(text); ++i) {
			text[i] = t[i];
		}
		text[i] = '\0';
	}
};

TEST_F(ProgressBarTest, testProgressRangeNesting) {
	RecordingProgress root;
	ProgressRange loading(root, 0.0f, 0.4f);
	ProgressRange textures(loading, 0.0f, 0.5f);
	textures.setProgress(1.0f);
	EXPECT_FLOAT_EQ(0.2f, root.last);

	ProgressRange models(loading, 0.5f, 1.0f);
	models.setProgress(0.5f);
	EXPECT_FLOAT_EQ(0.3f, root.last);
}

TEST_F(ProgressBarTest, testStepProgress) {
	RecordingProgress root;
	StepProgress steps(root, 5);

	steps.report(0, 0.0f);
	EXPECT_FLOAT_EQ(0.0f, root.last);
	steps.report(0, 0.5f);
	EXPECT_FLOAT_EQ(0.1f, root.last);
	steps.report(0, 1.0f);
	EXPECT_FLOAT_EQ(0.2f, root.last);

	steps.report(1, 0.0f);
	EXPECT_FLOAT_EQ(0.2f, root.last);
	steps.report(1, 0.5f);
	EXPECT_FLOAT_EQ(0.3f, root.last);

	ProgressRange step4 = steps.range(4);
	step4.setProgress(1.0f);
	EXPECT_FLOAT_EQ(1.0f, root.last);
}

TEST_F(ProgressBarTest, testNullProgress) {
	EXPECT_EQ(&NullProgress::get(), &progressOrNull(nullptr));
	RecordingProgress rec;
	EXPECT_EQ(&rec, &progressOrNull(&rec));
}

TEST_F(ProgressBarTest, testSharedProgress) {
	SharedProgress progress;
	progress.setText("load");
	progress.setProgress(0.25f);
	EXPECT_FLOAT_EQ(0.25f, progress.progress());
	EXPECT_STREQ("load", progress.text().c_str());

	progress.setProgress(2.0f);
	EXPECT_FLOAT_EQ(1.0f, progress.progress());

	progress.reset();
	EXPECT_FLOAT_EQ(0.0f, progress.progress());
	EXPECT_TRUE(progress.text().empty());
}

TEST_F(ProgressBarTest, testProgressScope) {
	EXPECT_EQ(nullptr, currentProgressPtr());
	EXPECT_EQ(&NullProgress::get(), &currentProgress());

	RecordingProgress outer;
	{
		ProgressScope scope(outer);
		EXPECT_EQ(&outer, currentProgressPtr());
		currentProgress().setProgress(0.25f);
		EXPECT_FLOAT_EQ(0.25f, outer.last);

		RecordingProgress inner;
		{
			ProgressScope nested(inner);
			EXPECT_EQ(&inner, currentProgressPtr());
			currentProgress().setProgress(0.75f);
			EXPECT_FLOAT_EQ(0.75f, inner.last);
			EXPECT_FLOAT_EQ(0.25f, outer.last);
		}
		EXPECT_EQ(&outer, currentProgressPtr());
	}
	EXPECT_EQ(nullptr, currentProgressPtr());
	EXPECT_EQ(&NullProgress::get(), &currentProgress());
}

TEST_F(ProgressBarTest, testParallelProgress) {
	RecordingProgress root;
	{
		ProgressScope scope(root);
		ParallelProgress progress(4);
		progress.add(1);
		EXPECT_FLOAT_EQ(0.25f, root.last);
		progress.add(1);
		EXPECT_FLOAT_EQ(0.5f, root.last);
		progress.add(2);
		EXPECT_FLOAT_EQ(1.0f, root.last);
	}
	EXPECT_EQ(nullptr, currentProgressPtr());
}

} // namespace core
