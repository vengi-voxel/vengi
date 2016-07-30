/**
 * @file
 */

#include "ui/tests/UITest.h"
#include "../ui/LoginWindow.h"
#include "../ui/SignupWindow.h"
#include "../ui/AuthFailedWindow.h"
#include "../ui/DisconnectWindow.h"
#include "../ui/HudWindow.h"

namespace ui {

class ClientUITest: public ui::UITest {
};

#if 0
TEST_F(ClientUITest, testLoginWindow) {
	new frontend::LoginWindow(nullptr);
}
#endif

TEST_F(ClientUITest, testSignupWindow) {
	ASSERT_DEATH(new frontend::SignupWindow(nullptr), "");
}


}
