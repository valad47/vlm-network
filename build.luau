return {
	pkgname = "network",
    pkgver = 1,
    luau_files = {
		"network.luau"
	},
    c_files = {
        "main.c"
    },
    c_args = {
        "-fPIC",
        "-lcurl"
    }
}
