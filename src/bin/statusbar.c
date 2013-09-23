#include <Elementary.h>
#include "common.h"

struct statusbar_s
{
   Evas_Object *layout;
   Eina_Stringshare *group_name;
   int cur_line;
   int max_line;
   config_data *cd;
};

void
stats_theme_change(stats_data *sd)
{
   elm_layout_file_set(sd->layout, EDJE_PATH, "statusbar_layout");
   stats_edc_file_set(sd, sd->group_name);
   stats_line_num_update(sd, sd->cur_line, sd->max_line);
   stats_view_size_update(sd);
   stats_cursor_pos_update(sd, 0, 0, 0, 0);
}

void
stats_line_num_update(stats_data *sd, int cur_line, int max_line)
{
   char buf[128];
   if (config_dark_theme_get(sd->cd))
     snprintf(buf, sizeof(buf),
              "<align=right>Line [<style=glow><color=#3399ff>%d</color></style>:<style=glow><color=#3399ff>%d</color></style>]</align>", cur_line, max_line);
   else
     snprintf(buf, sizeof(buf),
              "<align=right>Line [<color=#000000>%d</color>:<color=#000000>%d</color>]</align>", cur_line, max_line);
   elm_object_part_text_set(sd->layout, "elm.text.line", buf);
   sd->cur_line = cur_line;
   sd->max_line = max_line;
}

void
stats_edc_file_set(stats_data *sd, Eina_Stringshare *group_name)
{
   char buf[PATH_MAX];
   const char *filename = ecore_file_file_get(config_edc_path_get(sd->cd));
   if (config_dark_theme_get(sd->cd))
     snprintf(buf, sizeof(buf), "<align=right>File [<style=glow><color=#3399ff>%s</color></style>]    Group [<style=glow><color=#3399ff>%s</color></style>]</align>", filename, group_name);
   else
     snprintf(buf, sizeof(buf), "<align=right>File [<color=#000000>%s</color>]    Group [<color=#000000>%s</color>]</align>", filename, group_name);

   elm_object_part_text_set(sd->layout, "elm.text.file_group_name", buf);

   sd->group_name = eina_stringshare_add(group_name);
}

stats_data *
stats_init(Evas_Object *parent, config_data *cd)
{
   stats_data *sd = calloc(1, sizeof(stats_data));

   Evas_Object *layout = elm_layout_add(parent);
   elm_layout_file_set(layout, EDJE_PATH, "statusbar_layout");
   evas_object_show(layout);

   //FIXME: temporarily setup
   if (config_dark_theme_get(cd))
     elm_object_part_text_set(layout, "elm.text.cur_pos",
                              "Cursor [<style=glow><color=#3399ff>0</color></style>,<style=glow><color=#3399ff>0</color></style>] [<style=glow><color=#3399ff>0.00</color></style>,<style=glow><color=#3399ff>0.00</color></style>]");
   else
     elm_object_part_text_set(layout, "elm.text.cur_pos",
                              "Cursor [<color=#000000>0</color>,<color=#000000>0</color>] [<color=#000000>0.00</color>,<color=#000000>0.00</color>]");

   sd->layout = layout;
   sd->cd = cd;

   stats_edc_file_set(sd, NULL);

   return sd;
}

Evas_Object *
stats_obj_get(stats_data *sd)
{
   return sd->layout;
}

Eina_Stringshare *stats_group_name_get(stats_data *sd)
{
   return sd->group_name;
}

void
stats_term(stats_data *sd)
{
   if (!sd) return;
   eina_stringshare_del(sd->group_name);
   free(sd);
}

void
stats_info_msg_update(stats_data *sd, const char *msg)
{
   if (!config_stats_bar_get(sd->cd)) return;

   elm_object_part_text_set(sd->layout, "elm.text.info_msg", msg);
   elm_object_signal_emit(sd->layout, "elm,action,info_msg,show", "");
}

void
stats_view_size_update(stats_data *sd)
{
   Evas_Coord w, h;
   config_view_size_get(sd->cd, &w, &h);

   char buf[128];
   if (config_dark_theme_get(sd->cd))
     snprintf(buf, sizeof(buf),
              "Size [<style=glow><color=#3399ff>%d</color></style>x<style=glow><color=#3399ff>%d</color></style>]", w, h);
   else
     snprintf(buf, sizeof(buf),
              "Size [<color=#000000>%d</color>x<color=#000000>%d</color>]", w, h);

   elm_object_part_text_set(sd->layout, "elm.text.view_size", buf);
}

void
stats_cursor_pos_update(stats_data *sd, Evas_Coord x, Evas_Coord y, float rel_x, float rel_y)
{
   char buf[250];
   if (config_dark_theme_get(sd->cd))
     snprintf(buf, sizeof(buf),
              "Cursor [<style=glow><color=#3399ff>%d</color></style>,<style=glow><color=#3399ff>%d</color></style>] [<style=glow><color=#3399ff>%0.2f</color></style>,<style=glow><color=#3399ff>%0.2f</color></style>]", x, y, rel_x, rel_y);
   else
     snprintf(buf, sizeof(buf),
              "Cursor [<color=#000000>%d</color>,<color=#000000>%d</color>] [<color=#000000>%0.2f</color>,<color=#000000>%0.2f</color>]", x, y, rel_x, rel_y);

   elm_object_part_text_set(sd->layout, "elm.text.cur_pos", buf);
}
