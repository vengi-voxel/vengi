local tiles = {}

-- a defines arbitrary tiles
-- h is a horizontal street
-- v is a vertical street
-- x is a street cross

tiles.city = {
	['house1'] = {
		"0", "ha", "0",
		"va", "+a", "va",
		"0", "ha", "0"
	},
	['street_h_1'] = {
		"0",  "a", "0",
		"hx", "+h", "hx",
		"0",  "a", "0"
	},
	['street_v_1'] = {
		"0", "vx", "0",
		"a", "+v", "a",
		"0", "vx", "0"
	},
	['street_cross_1'] = {
		"0", "vx", "0",
		"hx", "+x", "hx",
		"0", "vx", "0"
	}
}

return tiles
