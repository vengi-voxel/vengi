function auth_failed()
	ui.windowPop()
	ui.rootWindow("main")
	ui.windowPush("popup", "Auth failed")
end
