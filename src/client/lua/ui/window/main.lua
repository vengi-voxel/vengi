local gui = require "ui.shared"
local style = require "ui.style"

local function login()
	gui.varStr('eMail', 'cl_email')
	gui.varStr('Password', 'cl_password')
	gui.row(2)
	gui.button('Login', function()
		client.auth(var.int('cl_email'), var.str('cl_password'))
	end)
	gui.button('Create new account', function()
		client.signup(var.str('cl_email'), var.str('cl_password'))
	end)
	gui.row(1)
	gui.button('Disconnect', function()
		client.disconnect()
	end)
end

local function connect()
	gui.varStr('Host', 'cl_host')
	gui.varStr('Port', 'cl_port')
	gui.row(1)
	gui.button('Connect to server', function()
		client.connect(var.int('cl_port'), var.str('cl_host'))
	end)
end

local function connecting()
	gui.row(1)
	ui.label('Connecting...')
	gui.row(1)
	gui.button('Disconnect', function()
		client.disconnect()
	end)
end

function main()
	gui.windowTitle('Main', 800, 400, function ()
		if client.isConnected() then
			login()
		elseif client.isConnecting() then
			connecting()
		else
			connect()
		end
		gui.row(1)
		gui.windowPushButton('Options', 'options')
		gui.row(1)
		gui.windowPushCommand('Quit', 'quit')
	end, 'border')
end
