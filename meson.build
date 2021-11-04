project('enventor', 'c',
  version : '1.0.99',
  meson_version : '>= 0.53',
  default_options : [
    'warning_level=2',
    'buildtype=debugoptimized',
    'c_std=c99'
  ]
)

v_array = meson.project_version().split('.')
v_maj = v_array[0]
v_min = v_array[1]
v_mic = v_array[2]

# install paths

dir_prefix = get_option('prefix')
dir_include = join_paths(dir_prefix, get_option('includedir'))
dir_pkginclude = join_paths(dir_include, meson.project_name())
dir_bin = join_paths(dir_prefix, get_option('bindir'))
dir_lib = join_paths(dir_prefix, get_option('libdir'))
dir_data = join_paths(dir_prefix, get_option('datadir'))
dir_pkgdata = join_paths(dir_data, meson.project_name())
dir_locale = join_paths(dir_prefix, get_option('localedir'))

# binaries

cc = meson.get_compiler('c')

enventor_cflags = []
enventor_cflags_try = [
  '-Wshadow',
  '-Wstrict-prototypes',
  '-Werror=missing-prototypes',
  '-Werror=pointer-arith',
  '-Wno-missing-field-initializers']

foreach cf: enventor_cflags_try
  if cc.has_argument(cf) == true
    enventor_cflags += cf
  endif
endforeach
add_global_arguments(enventor_cflags, language: 'c')

have_visibility_hidden = cc.has_argument('-fvisibility=hidden')
if have_visibility_hidden
  add_global_arguments('-fvisibility=hidden', language: 'c')
endif

pkgconfig = import('pkgconfig')
windows = import('windows')

eet = find_program('eet', native: true)
eet_exe = [ eet ]

edje_cc = find_program('edje_cc', native: true)
edje_cc_exe = [ edje_cc ]

# libraries

config_dir = [include_directories('.')]

efl_req = '>= 1.18.0'
enventor_deps = [
  dependency('eina', version : efl_req),
  dependency('eo', version : efl_req),
  dependency('efl', version : efl_req),
  dependency('eet', version : efl_req),
  dependency('ecore', version : efl_req),
  dependency('ecore-file', version : efl_req),
  dependency('evas', version : efl_req),
  dependency('eio', version : efl_req),
  dependency('efreet', version : efl_req),
  dependency('edje', version : efl_req),
  dependency('elementary', version : efl_req)
]

requirement_enventor_pc = ' eina ' + efl_req + ' eo ' + efl_req + ' efl ' + efl_req + ' eet ' + efl_req + ' ecore ' + efl_req + ' ecore-file ' + efl_req + ' evas ' + efl_req + ' eio ' + efl_req + ' efreet ' + efl_req + ' edje ' + efl_req + ' elementary ' + efl_req

enventor_cargs = [
  '-D_POSIX_C_SOURCE=200809L',
  '-D_XOPEN_SOURCE=500' ]

have_nls = false
if get_option('nls') == true
  intl_dep = cc.find_library('intl', required: false)
  if intl_dep.found() == true and cc.has_header('libintl.h') == true
    enventor_deps += cc.find_library('intl', required: true)
    enventor_cargs += '-DENABLE_NLS'
    have_nls = true
  endif
endif

# configuration

config_h = configuration_data()
config_h.set_quoted('PACKAGE_NAME', meson.project_name())
config_h.set_quoted('PACKAGE_VERSION', meson.project_version())
config_h.set_quoted('PACKAGE_BIN_DIR', dir_bin)
config_h.set_quoted('PACKAGE_LIB_DIR', dir_lib)
config_h.set_quoted('PACKAGE_DATA_DIR', dir_pkgdata)
config_h.set_quoted('LOCALE_DIR', dir_locale)
config_h.set('EFL_BETA_API_SUPPORT', '1')
config_h.set('EFL_UI_FOCUS_OBJECT_PROTECTED', '1')
config_h.set('ENVENTOR_BETA_API_SUPPORT', '1')

subdir('src/lib')
subdir('src/bin')
subdir('data/autocomp')
subdir('data/color')
subdir('data/desktop')
subdir('data/help')
subdir('data/icon')
subdir('data/images')
subdir('data/reference')
subdir('data/sounds')
subdir('data/templates')
subdir('data/themes/default')
subdir('pc')
if have_nls == true
  subdir('po')
endif

install_data(
  sources     : 'README',
  install_dir : join_paths(dir_pkgdata, 'docs')
)

# Use config_h after all subdirs have set values

configure_file(output : 'config.h', configuration : config_h)

# output

summary({'OS': host_machine.system(),
         'NLS': have_nls ? 'yes' : 'no'
        }, section: 'Configuration Options Summary:')

summary({'prefix': dir_prefix,
         'bindir': dir_bin,
         'libdir': dir_lib,
         'incdir': dir_include,
         'pkgincdir': dir_pkginclude,
         'datadir': dir_data,
         'pkgdatadir': dir_pkgdata,
         'localedir': dir_locale,
        }, section: 'Directories:')

summary({'compilation': 'ninja',
         'installation': 'ninja install',
        }, section: 'Compilation')
