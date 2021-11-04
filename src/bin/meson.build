
enventor_bin_src = [
  'base_gui.c',
  'build_setting.c',
  'config_data.c',
  'console.c',
  'edc_navigator.c',
  'file_browser.c',
  'file_mgr.c',
  'file_tab.c',
  'goto.c',
  'help.c',
  'live_edit.c',
  'main.c',
  'menu.c',
  'newfile.c',
  'panes.c',
  'preference_setting.c',
  'search.c',
  'setting.c',
  'statusbar.c',
  'text_setting.c',
  'tools.c',
  'utils.c'
]

enventor_bin = executable('enventor', enventor_bin_src,
  c_args : [ enventor_cargs,
             '-DHAVE_CONFIG_H',
             '-I' + join_paths(meson.source_root(), 'src/include') ],
  dependencies : [ enventor,  dependency('elementary', version : efl_req) ],
  include_directories : config_dir,
  install : true
)
