project('shell', 'c',
        version: '1.7.1',
        meson_version: '>= 1.3.0',
	default_options: [
		'c_std=c17'
	]
)
cc = meson.get_compiler('c')
conf = configuration_data()
cc.has_header('termios.h', required: true)
cc.has_header('unistd.h', required: true)
foreach func : ['fork', 'malloc', 'realloc', 'getcwd', 'strtol', 'getline']
	cc.has_function(func, required: true)
endforeach
subdir('src')
