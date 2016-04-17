/*****************************************************************************/
/* Externally accessible calls                                               */
/*****************************************************************************/
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "common.h"
#include "text_setting.h"

typedef enum {
   SETTING_VIEW_PREFERENCE = 0,
   SETTING_VIEW_TEXT,
   SETTING_VIEW_BUILD,
   SETTING_VIEW_NONE
} setting_view;

typedef struct setting_s
{
   Evas_Object *layout;
   Evas_Object *toolbar;

   preference_setting_data *psd;
   text_setting_data *tsd;
   build_setting_data *bsd;

   setting_view current_view;

   Evas_Object *apply_btn;
   Evas_Object *reset_btn;
   Evas_Object *cancel_btn;

} setting_data;

static setting_data *g_sd = NULL;


/*****************************************************************************/
/* Internal method implementation                                            */
/*****************************************************************************/

static void
setting_dismiss_done_cb(void *data, Evas_Object *obj EINA_UNUSED,
                        const char *emission EINA_UNUSED,
                        const char *source EINA_UNUSED)
{
   setting_data *sd = data;

   text_setting_term(sd->tsd);
   build_setting_term(sd->bsd);

   evas_object_del(sd->layout);
   sd->layout = NULL;
   menu_deactivate_request();

   free(sd);
   g_sd = NULL;
}

static void
setting_apply_btn_cb(void *data, Evas_Object *obj EINA_UNUSED,
                     void *event_info EINA_UNUSED)
{
   setting_data *sd = data;

   preference_setting_config_set(sd->psd);
   build_setting_config_set(sd->bsd);
   text_setting_config_set(sd->tsd);

   config_apply();

   setting_close();
}

static void
setting_cancel_btn_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                      void *event_info EINA_UNUSED)
{
   setting_close();
}

static void
setting_reset_btn_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                      void *event_info EINA_UNUSED)
{
   setting_data *sd = data;

   preference_setting_reset(sd->psd);
   text_setting_reset(sd->tsd);
   build_setting_reset(sd->bsd);
}

static void
focus_custom_chain_set(setting_data *sd, Evas_Object *content)
{
   //Set a custom chain to set the focus order.
   Eina_List *custom_chain = NULL;
   custom_chain = eina_list_append(custom_chain, sd->toolbar);
   custom_chain = eina_list_append(custom_chain, content);
   custom_chain = eina_list_append(custom_chain, sd->apply_btn);
   custom_chain = eina_list_append(custom_chain, sd->reset_btn);
   custom_chain = eina_list_append(custom_chain, sd->cancel_btn);
   elm_object_focus_custom_chain_set(sd->layout, custom_chain);
}

static void
toolbar_preferene_cb(void *data, Evas_Object *obj EINA_UNUSED,
                       void *event_info EINA_UNUSED)
{
   setting_data *sd = data;

   if (sd->current_view == SETTING_VIEW_PREFERENCE) return;

   //Hide previous tab view
   Evas_Object *pcontent = elm_object_part_content_unset(sd->layout,
                                                        "elm.swallow.content");
   evas_object_hide(pcontent);

   Evas_Object *content = preference_setting_content_get(sd->psd, obj);
   elm_object_part_content_set(sd->layout, "elm.swallow.content",
                               content);
   focus_custom_chain_set(sd, content);
   preference_setting_focus_set(sd->psd);

   sd->current_view = SETTING_VIEW_PREFERENCE;
}

static void
toolbar_text_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   setting_data *sd = data;

   if (sd->current_view == SETTING_VIEW_TEXT) return;

   if (!sd->tsd) sd->tsd = text_setting_init();

   //Hide previous tab view
   Evas_Object *pcontent = elm_object_part_content_unset(sd->layout,
                                                        "elm.swallow.content");
   evas_object_hide(pcontent);

   Evas_Object *content = text_setting_content_get(sd->tsd, obj);
   elm_object_part_content_set(sd->layout, "elm.swallow.content",
                               content);
   focus_custom_chain_set(sd, content);
   text_setting_focus_set(sd->tsd);

   sd->current_view = SETTING_VIEW_TEXT;
}

static void
toolbar_build_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   setting_data *sd = data;

   if (sd->current_view == SETTING_VIEW_BUILD) return;

   if (!sd->bsd) sd->bsd = build_setting_init();

   //Hide previous tab view
   Evas_Object *pcontent = elm_object_part_content_unset(sd->layout,
                                                        "elm.swallow.content");
   evas_object_hide(pcontent);

   Evas_Object *content = build_setting_content_get(sd->bsd, obj);
   elm_object_part_content_set(sd->layout, "elm.swallow.content",
                               content);
   focus_custom_chain_set(sd, content);
   build_setting_focus_set(sd->bsd);

   sd->current_view = SETTING_VIEW_BUILD;
}


/*****************************************************************************/
/* Externally accessible calls                                               */
/*****************************************************************************/

void
setting_open(void)
{
   setting_data *sd = g_sd;
   if (sd) return;

   sd = calloc(1, sizeof(setting_data));
   if (!sd)
     {
        EINA_LOG_ERR(_("Failed to allocate Memory!"));
        return;
     }
   g_sd = sd;

   search_close();
   goto_close();

   //Layout
   Evas_Object *layout = elm_layout_add(base_win_get());
   elm_layout_file_set(layout, EDJE_PATH, "setting_layout");
   elm_object_signal_callback_add(layout, "elm,state,dismiss,done", "",
                                  setting_dismiss_done_cb, sd);
   evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_layout_text_set(layout, "title_name", _("Settings"));
   evas_object_show(layout);
   base_win_resize_object_add(layout);

   //Tabbar
   Evas_Object *toolbar = elm_toolbar_add(layout);
   //elm_object_style_set(toolbar, "item_horizontal");
   elm_toolbar_shrink_mode_set(toolbar, ELM_TOOLBAR_SHRINK_EXPAND);
   elm_toolbar_select_mode_set(toolbar, ELM_OBJECT_SELECT_MODE_ALWAYS);
   evas_object_size_hint_weight_set(toolbar, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);

   elm_toolbar_item_append(toolbar, NULL,  _("Preferences"),
                           toolbar_preferene_cb, sd);
   elm_toolbar_item_append(toolbar, NULL, _("Text Editor"), toolbar_text_cb,
                           sd);
   elm_toolbar_item_append(toolbar, NULL, _("EDC Build"), toolbar_build_cb,
                           sd);

   elm_object_part_content_set(layout, "elm.swallow.toolbar", toolbar);

   //Preference Content as a default
   sd->psd = preference_setting_init();
   Evas_Object *content = preference_setting_content_get(sd->psd, layout);
   elm_object_part_content_set(layout, "elm.swallow.content", content);
   preference_setting_focus_set(sd->psd);
   sd->current_view = SETTING_VIEW_PREFERENCE;

   //Apply Button
   Evas_Object *apply_btn = elm_button_add(layout);
   elm_object_text_set(apply_btn, _("Apply"));
   evas_object_smart_callback_add(apply_btn, "clicked", setting_apply_btn_cb,
                                  sd);
   elm_object_part_content_set(layout, "elm.swallow.apply_btn", apply_btn);

   //Reset Button
   Evas_Object *reset_btn = elm_button_add(layout);
   elm_object_text_set(reset_btn, _("Reset"));
   evas_object_smart_callback_add(reset_btn, "clicked", setting_reset_btn_cb,
                                  sd);
   elm_object_part_content_set(layout, "elm.swallow.reset_btn", reset_btn);

   //Cancel Button
   Evas_Object *cancel_btn = elm_button_add(layout);
   elm_object_text_set(cancel_btn, _("Cancel"));
   evas_object_smart_callback_add(cancel_btn, "clicked", setting_cancel_btn_cb,
                                  sd);
   elm_object_part_content_set(layout, "elm.swallow.cancel_btn", cancel_btn);

   sd->layout = layout;
   sd->toolbar = toolbar;
   sd->apply_btn = apply_btn;
   sd->reset_btn = reset_btn;
   sd->cancel_btn = cancel_btn;

   menu_activate_request();
}

void
setting_close()
{
   setting_data *sd = g_sd;
   if (!sd) return;
   elm_object_signal_emit(sd->layout, "elm,state,dismiss", "");
}

Eina_Bool
setting_is_opened(void)
{
   setting_data *sd = g_sd;
   if (!sd) return EINA_FALSE;
   return EINA_TRUE;
}
