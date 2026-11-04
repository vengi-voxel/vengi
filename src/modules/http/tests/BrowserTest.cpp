/**
 * @file
 * @brief Browser integration test. Serve with tests/server.py, then open the generated HTML.
 */

#include "http/Client.h"
#include <SDL3/SDL.h>
#include <emscripten.h>

namespace {

http::Client *client;
int cancelledCalls = 0;
int completed = 0;
bool failed = false;
http::RequestId slowRequest;
int frames = 0;

void tick() {
	http::update();
	++frames;
	if (frames == 1) {
		client->cancel(slowRequest);
	}
	if (completed == 3 || frames > 600) {
		const bool pass = completed == 3 && cancelledCalls == 0 && !failed;
		http::shutdown();
		delete client;
		emscripten_run_script(pass ? "document.body.textContent='HTTP_BROWSER_PASS'"
								   : "document.body.textContent='HTTP_BROWSER_FAIL'");
		emscripten_cancel_main_loop();
	}
}

} // namespace

int main() {
	SDL_Init(0);
	http::init();
	client = new http::Client();
	{
		const char bytes[] = {'a', '\0', 'b', (char)0xff};
		http::Request post("/echo", http::RequestType::POST);
		post.setBody(bytes, sizeof(bytes));
		post.addHeader("X-Test", "browser-http");
		client->execute(post, [](const http::Response &response) {
			const char expected[] = {'a', '\0', 'b', (char)0xff};
			core::String header;
			failed |= !response.success || response.statusCode != 404 ||
					  (response.body.size() != sizeof(expected) ||
					   SDL_memcmp(response.body.data(), expected, sizeof(expected)) != 0) ||
					  !response.headers.get("x-test", header) || header != "browser-http";
			++completed;
		});
		http::Request missing("/missing", http::RequestType::GET);
		client->execute(missing, [](const http::Response &response) {
			failed |= !response.success || response.statusCode != 404 || response.text() != "missing";
			++completed;
		});
		// Serve this test on 127.0.0.1 to exercise a different origin on localhost.
		http::Request noCache(emscripten_run_script_string(
			"location.protocol + '//localhost:' + location.port + '/cors-no-cache'"), http::RequestType::GET);
		noCache.noCache();
		client->execute(noCache, [](const http::Response &response) {
			failed |= !response.success || response.statusCode != 200 || response.text() != "no-cache";
			++completed;
		});
		http::Request slow("/slow", http::RequestType::GET);
		slowRequest = client->execute(slow, [](const http::Response &) { ++cancelledCalls; });
		http::Client temporary;
		temporary.execute(slow, [](const http::Response &) { ++cancelledCalls; });
		http::update();
	}
	emscripten_set_main_loop(tick, 0, false);
	return 0;
}
