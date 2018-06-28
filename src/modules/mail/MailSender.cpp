/**
 * @file
 */

#include "MailSender.h"
#include "core/Log.h"
#include "core/Var.h"
#include <curl/curl.h>

namespace mail {

struct Message {
	const std::string& payload;
	int position = 0;
};

constexpr static auto TIMEOUT = 30;

size_t MailSender::readFn(void *ptr, size_t size, size_t nmemb, void *userp) {
	const size_t bytes = size * nmemb;
	if (bytes < 1) {
		return 0;
	}
	Message* msg = (Message*)userp;
	const size_t len = msg->payload.size() - msg->position;
	if (len == 0) {
		return 0;
	}
	if (len > bytes) {
		memcpy(ptr, &msg->payload[msg->position], bytes);
		msg->position += bytes;
		return bytes;
	}
	memcpy(ptr, &msg->payload[msg->position], len);
	return len;
}

bool MailSender::setProxy(const std::string& host, uint16_t port, const char *user, const char *password) {
	_proxyHost = host;
	_proxyPort = port;
	if (user != nullptr) {
		if (password == nullptr) {
			return false;
		}
		_proxyUser = user;
		_proxyPassword = password;
		_proxyUserPwd = _proxyUser + ":" + _proxyPassword;
	} else {
		_proxyUser = "";
		_proxyPassword = "";
		_proxyUserPwd = "";
	}
	return true;
}

bool MailSender::send(const char *recipient, const char *subject,
		const char *body) {
	CURL* curl = curl_easy_init();
	if (curl == nullptr) {
		Log::error("Failed to init curl - can't send mail");
		return false;
	}

	time_t t = time(0);
	const struct tm* now = localtime(&t);
	char date[32];
	strftime(date, sizeof(date), "%a, %d %b %G %T %z", now);

	const std::string& payloadStr = core::string::format(
		"Date: %s\r\n"
		"To: %s\r\n"
		"From: %s\r\n"
		"Subject: %s\r\n"
		"\r\n"
		"%s\r\n",
		date, recipient, _from->strVal().c_str(), subject, body
	);

	Message msg { payloadStr, 0 };

	curl_easy_setopt(curl, CURLOPT_USERNAME, _user->strVal().c_str());
	curl_easy_setopt(curl, CURLOPT_PASSWORD, _password->strVal().c_str());
	curl_easy_setopt(curl, CURLOPT_MAIL_FROM, _from->strVal().c_str());
	curl_easy_setopt(curl, CURLOPT_URL, _url->strVal().c_str());
	curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, TIMEOUT);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, TIMEOUT);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
#ifdef DEBUG
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
#endif

	if (!_proxyHost.empty()) {
		curl_easy_setopt(curl, CURLOPT_PROXY, _proxyHost.c_str());
		curl_easy_setopt(curl, CURLOPT_PROXYPORT, _proxyPort);
		curl_easy_setopt(curl, CURLOPT_PROXYTYPE, CURLPROXY_SOCKS5_HOSTNAME);
		if (!_proxyUserPwd.empty()) {
			curl_easy_setopt(curl, CURLOPT_PROXYAUTH, CURLAUTH_ANY);
			curl_easy_setopt(curl, CURLOPT_PROXYUSERPWD, _proxyUserPwd.c_str());
		}
	}

	struct curl_slist *recipients = nullptr;
	recipients = curl_slist_append(recipients, recipient);
	curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

#ifdef __USE_XOPEN2K8
	FILE *f = fmemopen(const_cast<char *>(msg.payload.c_str()), msg.payload.size(), "r");
	curl_easy_setopt(curl, CURLOPT_READDATA, f);
#else
	curl_easy_setopt(curl, CURLOPT_READFUNCTION, readFn);
	curl_easy_setopt(curl, CURLOPT_READDATA, &msg);
#endif
	curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

	const CURLcode res = curl_easy_perform(curl);
	if (res != CURLE_OK) {
		Log::error("Failed to send mail with subject '%s': %s", subject,
				curl_easy_strerror(res));
	}
	curl_slist_free_all(recipients);
	curl_easy_cleanup(curl);
#ifdef __USE_XOPEN2K8
	fclose(f);
#endif
	return res == CURLE_OK;
}

void MailSender::construct() {
	_url = core::Var::getSafe("smtp_url");
	_user = core::Var::getSafe("smtp_user");
	_from = core::Var::getSafe("smtp_from");
	_password = core::Var::getSafe("smtp_password");
}

bool MailSender::init() {
	curl_version_info_data* versionInfo = curl_version_info(CURLVERSION_NOW);
	if (versionInfo == nullptr) {
		Log::error("Could not query version information for libcurl");
		return false;
	}
	if ((versionInfo->features & CURL_VERSION_SSL) != CURL_VERSION_SSL) {
		Log::error("No ssl support compiled into libcurl");
		return false;
	}
	return true;
}

void MailSender::shutdown() {
}

}
