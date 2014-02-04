#include <Elementary.h>
#include "common.h"

typedef struct statusbar_s
{
   Evas_Object *layout;
   Eina_Stringshare *group_name;
   int cur_line;
   int max_line;
} stats_data;

stats_data *g_sd = NULL;

void
stats_line_num_update(int cur_line, int max_line)
{
   stats_data *sd = g_sd;

   char buf[128];
   snprintf(buf, sizeof(buf),
            "<align=right>Line [<style=glow><color=#3399ff>%d</color></style>:<style=glow><color=#3399ff>%d</color></style>]</align>", cur_line, max_line);
   elm_object_part_text_set(sd->layout, "elm.text.line", buf);
   sd->cur_line = cur_line;
   sd->max_line = max_line;
}

void
stats_edc_group_set(Eina_Stringshare *group_name)
{
   stats_data *sd = g_sd;

   char buf[1028];
   snprintf(buf, sizeof(buf), "<align=right>Group [<style=glow><color=#3399ff>%s</color></style></align>", group_name);
   elm_object_part_text_set(sd->layout, "elm.text.group_name", buf);
   sd->group_name = eina_stringshare_add(group_name);
}

Evas_Object *
stats_init(Evas_Object *parent)
{
   stats_data *sd = calloc(1, sizeof(stats_data));
   g_sd = sd;

   Evas_Object *layout = elm_layout_add(parent);
   elm_layout_file_set(layout, EDJE_PATH, "statusbar_layout");
   evas_object_show(layout);

   sd->layout = layout;

   stats_cursor_pos_update(0, 0, 0, 0);
   stats_edc_group_set(NULL);

   return layout;
}

Eina_Stringshare *stats_group_name_get()
{
   stats_data *sd = g_sd;
   return sd->group_name;
}

void
stats_term()
{
   stats_data *sd = g_sd;
   eina_stringshare_del(sd->group_name);
   free(sd);
}

void
stats_info_msg_update(const char *msg)
{
   if (!config_stats_bar_get()) return;

   stats_data *sd = g_sd;
   elm_object_part_text_set(sd->layout, "elm.text.info_msg", msg);
   elm_object_signal_emit(sd->layout, "elm,action,info_msg,show", "");
}

void
stats_view_size_update()
{
   stats_data *sd = g_sd;

   Evas_Coord w, h;
   config_view_size_get(&w, &h);

   char buf[128];
   snprintf(buf, sizeof(buf),
            "Size [<style=glow><color=#3399ff>%d</color></style>x<style=glow><color=#3399ff>%d</color></style>]", w, h);

   elm_object_part_text_set(sd->layout, "elm.text.view_size", buf);
}

void
stats_cursor_pos_update(Evas_Coord x, Evas_Coord y, float rel_x, float rel_y)
{
   stats_data *sd = g_sd;
   char buf[250];
   snprintf(buf, sizeof(buf),
            "Cursor [<style=glow><color=#3399ff>%d</color></style>,<style=glow><color=#3399ff>%d</color></style>] [<style=glow><color=#3399ff>%0.2f</color></style>,<style=glow><color=#3399ff>%0.2f</color></style>]", x, y, rel_x, rel_y);

   elm_object_part_text_set(sd->layout, "elm.text.cur_pos", buf);
}
