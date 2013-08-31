#ifndef __TEMPLATE_CODE_H__
#define __TEMPLATE_CODE_H__

#define TEMPLATE_PART_LINE_CNT 11

const char *TEMPLATE_PART[TEMPLATE_PART_LINE_CNT] =
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

#define TEMPLATE_DESC_LINE_CNT 4

const char *TEMPLATE_DESC[TEMPLATE_DESC_LINE_CNT] =
{
   "description { state: \"template\" 0.0;\n",
   "   inherit: \"default\" 0.0;\n",
   "   visible: 1;\n",
   "}\n"
};

#define TEMPLATE_PROG_LINE_CNT 6

const char *TEMPLATE_PROG[TEMPLATE_PROG_LINE_CNT] =
{
   "program { name: \"template\";\n",
   "   signal: \"*\";\n",
   "   source: \"*\";\n",
   "   action: STATE_SET \"default\" 0.0;\n",
   "   target: \"template\";\n",
   "}\n"
};

#endif
