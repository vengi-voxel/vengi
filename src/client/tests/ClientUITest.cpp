/**
 * @file
 */

#include "ui/tests/UITest.h"
#include "../ui/LoginWindow.h"
#include "../ui/SignupWindow.h"
#include "../ui/AuthFailedWindow.h"
#include "../ui/DisconnectWindow.h"
#include "../ui/LostPasswordWindow.h"
#include "../ui/HudWindow.h"

namespace ui {

class ClientUITest: public ui::UITest {
};

TEST_F(ClientUITest, testLoadClientUI) {
	//ASSERT_DEATH(new frontend::LoginWindow(nullptr), "");
	//ASSERT_DEATH(new frontend::SignupWindow(nullptr), "");
	ASSERT_DEATH(new frontend::AuthFailedWindow(nullptr), "");
	ASSERT_DEATH(new frontend::DisconnectWindow(nullptr), "");
	//ASSERT_DEATH(new frontend::HudWindow(nullptr, glm::ivec2()), "");
	//ASSERT_DEATH(new frontend::LostPasswordWindow(nullptr), "");
}


}
