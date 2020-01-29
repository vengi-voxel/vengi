/**
 * @file
 */

#include "core/IComponent.h"
#include "core/Var.h"

namespace mail {

/**
 * @brief Send mails via smtp
 */
class MailSender: public core::IComponent {
private:
	core::VarPtr _url;
	core::VarPtr _user;
	core::VarPtr _from;
	core::VarPtr _password;

	core::String _proxyHost;
	core::String _proxyUser;
	core::String _proxyPassword;
	core::String _proxyUserPwd;
	uint16_t _proxyPort;

	static size_t readFn(void *ptr, size_t size, size_t nmemb, void *userp);
public:
	void construct() override;
	bool init() override;
	void shutdown() override;

	bool setProxy(const core::String& host, uint16_t port = 3128, const char *user = nullptr, const char *password = nullptr);

	bool send(const char *recipient, const char *subject, const char *body);
};

}
