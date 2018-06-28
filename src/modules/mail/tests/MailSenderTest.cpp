/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "mail/MailSender.h"

namespace mail {

class MailSenderTest : public core::AbstractTest {
};

TEST_F(MailSenderTest, testSend) {
	core::Var::get("smtp_url", "smtp://smtp.gmail.com");
	const core::VarPtr& passwd = core::Var::get("smtp_password");
	if (!passwd) {
		Log::warn("At least one of 'smtp_url', 'smtp_from', 'smtp_user' or 'smtp_password' is not specified, can't execute test");
		return;
	}

	MailSender sender;
	sender.construct();
	ASSERT_TRUE(sender.init());

	// send to sender
	const std::string& to = core::Var::get("smtp_from")->strVal();
	ASSERT_TRUE(sender.send(to.c_str(), "Test", "This is a test mail\nwith some content."));

	sender.shutdown();
}

}
