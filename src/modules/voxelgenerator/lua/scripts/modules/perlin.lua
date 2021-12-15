local module = {}

module.p = {}
module.permutation = {
	151,
	160,
	137,
	91,
	90,
	15,
	131,
	13,
	201,
	95,
	96,
	53,
	194,
	233,
	7,
	225,
	140,
	36,
	103,
	30,
	69,
	142,
	8,
	99,
	37,
	240,
	21,
	10,
	23,
	190,
	6,
	148,
	247,
	120,
	234,
	75,
	0,
	26,
	197,
	62,
	94,
	252,
	219,
	203,
	117,
	35,
	11,
	32,
	57,
	177,
	33,
	88,
	237,
	149,
	56,
	87,
	174,
	20,
	125,
	136,
	171,
	168,
	68,
	175,
	74,
	165,
	71,
	134,
	139,
	48,
	27,
	166,
	77,
	146,
	158,
	231,
	83,
	111,
	229,
	122,
	60,
	211,
	133,
	230,
	220,
	105,
	92,
	41,
	55,
	46,
	245,
	40,
	244,
	102,
	143,
	54,
	65,
	25,
	63,
	161,
	1,
	216,
	80,
	73,
	209,
	76,
	132,
	187,
	208,
	89,
	18,
	169,
	200,
	196,
	135,
	130,
	116,
	188,
	159,
	86,
	164,
	100,
	109,
	198,
	173,
	186,
	3,
	64,
	52,
	217,
	226,
	250,
	124,
	123,
	5,
	202,
	38,
	147,
	118,
	126,
	255,
	82,
	85,
	212,
	207,
	206,
	59,
	227,
	47,
	16,
	58,
	17,
	182,
	189,
	28,
	42,
	223,
	183,
	170,
	213,
	119,
	248,
	152,
	2,
	44,
	154,
	163,
	70,
	221,
	153,
	101,
	155,
	167,
	43,
	172,
	9,
	129,
	22,
	39,
	253,
	19,
	98,
	108,
	110,
	79,
	113,
	224,
	232,
	178,
	185,
	112,
	104,
	218,
	246,
	97,
	228,
	251,
	34,
	242,
	193,
	238,
	210,
	144,
	12,
	191,
	179,
	162,
	241,
	81,
	51,
	145,
	235,
	249,
	14,
	239,
	107,
	49,
	192,
	214,
	31,
	181,
	199,
	106,
	157,
	184,
	84,
	204,
	176,
	115,
	121,
	50,
	45,
	127,
	4,
	150,
	254,
	138,
	236,
	205,
	93,
	222,
	114,
	67,
	29,
	24,
	72,
	243,
	141,
	128,
	195,
	78,
	66,
	215,
	61,
	156,
	180
}
module.size = 256
module.gx = {}
module.gy = {}
module.randMax = 256

function module:load()
	for i = 1, self.size do
		self.p[i] = self.permutation[i]
		self.p[256 + i] = self.p[i]
	end
end

function module:noise(x, y, z)
	local X = math.floor(x) % 256
	local Y = math.floor(y) % 256
	local Z = math.floor(z) % 256
	x = x - math.floor(x)
	y = y - math.floor(y)
	z = z - math.floor(z)
	local u = module.fade(x)
	local v = module.fade(y)
	local w = module.fade(z)
	local A = self.p[X + 1] + Y
	local AA = self.p[A + 1] + Z
	local AB = self.p[A + 2] + Z
	local B = self.p[X + 2] + Y
	local BA = self.p[B + 1] + Z
	local BB = self.p[B + 2] + Z

	return module.lerp(
		w,
		module.lerp(
			v,
			module.lerp(u, module.grad(self.p[AA + 1], x, y, z), module.grad(self.p[BA + 1], x - 1, y, z)),
			module.lerp(u, module.grad(self.p[AB + 1], x, y - 1, z), module.grad(self.p[BB + 1], x - 1, y - 1, z))
		),
		module.lerp(
			v,
			module.lerp(u, module.grad(self.p[AB + 2], x, y, z - 1), module.grad(self.p[BA + 2], x - 1, y, z - 1)),
			module.lerp(u, module.grad(self.p[AB + 2], x, y - 1, z - 1), module.grad(self.p[BB + 2], x - 1, y - 1, z - 1))
		)
	)
end

function module.fade(t)
	return t * t * t * (t * (t * 6 - 15) + 10)
end

function module.lerp(t, a, b)
	return a + t * (b - a)
end

function module.grad(hash, x, y, z)
	local h = hash % 16
	local u = h < 8 and x or y
	local v = h < 4 and y or ((h == 12 or h == 14) and x or z)
	return ((h % 2) == 0 and u or -u) + ((h % 3) == 0 and v or -v)
end

--[[
normalize the noise range from [-1/1] to [0/1]
--]]
function module:norm(v)
	return (v + 1.0) / 2.0
end

return module
