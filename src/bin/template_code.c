#define TEMPLATE_RECT_LINE_CNT 11

const char *TEMPLATE_RECT[TEMPLATE_RECT_LINE_CNT] =
{
   "part { name: \"template\";\n",
   "   type: RECT;\n",
   "   scale: 1;\n",
   "   mouse_events: 1;\n",
   "   description { state: \"default\" 0.0;\n",
   "      rel1.relative: 0.0 0.0;\n",
   "      rel2.relative: 1.0 1.0;\n",
   "      color: 255 255 255 255;\n",
   "      align: 0.5 0.5;\n",
   "   }\n",
   "}\n"
};
