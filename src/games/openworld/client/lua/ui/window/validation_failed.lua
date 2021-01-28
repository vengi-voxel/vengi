function validation_failed()
	ui.windowPop()
	ui.rootWindow("main")
	ui.windowPush("popup", "Validation token or email is not valid")
end
