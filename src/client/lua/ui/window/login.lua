local gui = require "ui.shared"

function login()
	gui.windowTitle('Login', 900, 600, function ()
		gui.varStr('eMail', 'cl_email')
		gui.varStr('Password', 'cl_password')
		gui.varStr('Host', 'cl_host')
		gui.varStr('Port', 'cl_port')
		gui.varCheckbox('Autologin', 'cl_autologin')
		gui.row(1)
		gui.button('Login', function()
			if client.connect(var.int('cl_port'), var.str('cl_host')) then
				ui.rootWindow("hud")
			end
		end)
		gui.back()
	end, 'border', 'title')
end
