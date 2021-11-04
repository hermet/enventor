
if get_option('nls')
    i18n = import('i18n')
    i18n.gettext(meson.project_name(),
          args: [
            '--directory=' + meson.source_root(),
            '--keyword=_',
            '--keyword=d_:1',
            '--keyword=P_:1,2',
            '--keyword=dP_:1,2',
            '--keyword=N_',
            '--keyword=NP_:1,2',
            '--from-code=UTF-8',
            '--foreign-user'
            ])
  config_h.set('ENABLE_NLS', '1')
endif
