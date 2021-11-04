
edc_theme_files = [
  'enventor.edc'
]

enventor_themes = []

_edje_cc = find_program('edje_cc', native: true)
edje_cc_exe = [_edje_cc]

foreach edc_file : edc_theme_files
  enventor_themes += custom_target('edje_cc_' + edc_file,
    input       : edc_file,
    depfile     : '@BASENAME@.edj.d',
    output      : '@BASENAME@.edj',
    command     : edje_cc_exe +
                  [
                   '-sd', join_paths(meson.current_source_dir(), 'sounds'),
                   '-id', join_paths(meson.current_source_dir(), 'images'),
                   '@INPUT@', '@OUTPUT@'
                  ],
    install     : true,
    install_dir : join_paths(dir_pkgdata, 'themes'),
  )
endforeach
