#define TEMPLATE_GROUP_LINE_CNT 28

const char *TEMPLATE_GROUP[TEMPLATE_GROUP_LINE_CNT] =
{
   "group { name: \"XXX\";<br/>",
   "   parts {<br/>",
   "      part { name: \"XXX\";<br/>",
   "         type: IMAGE;<br/>",
   "         scale: 1;<br/>",
   "         mouse_events: 1;<br/>",
   "         description { state: \"default\" 0.0;<br/>",
   "            rel1 { relative: 0.0 0.0; offset: 0 0; /*to: \"XXX\";*/ }<br/>",
   "            rel2 { relative: 1.0 1.0; offset: -1 -1; /*to: \"XXX\";*/ }<br/>",
   "            align: 0.5 0.5;<br/>",
   "            fixed: 0 0;<br/>",
   "            min: 0 0;<br/>",
   "            visible: 1;<br/>",
   "            image.normal: \"logo.png\";<br/>",
   "            //aspect: 1 1;<br/>",
   "         }<br/>",
   "      }<br/>",
   "   }<br/>",
   "   programs {<br/>",
   "      program { name: \"XXX\";<br/>",
   "         //signal: \"XXX\";<br/>",
   "         //source: \"XXX\";<br/>",
   "         action: STATE_SET \"default\" 0.0;<br/>",
   "         target: \"XXX\";<br/>",
   "         //transition: LINEAR 1.0;<br/>",
   "      }<br/>",
   "   }<br/>",
   "}"
};

#define TEMPLATE_PART_IMAGE_LINE_CNT 15

const char *TEMPLATE_PART_IMAGE[TEMPLATE_PART_IMAGE_LINE_CNT] =
{
   "   type: IMAGE;<br/>",
   "   scale: 1;<br/>",
   "   mouse_events: 1;<br/>",
   "   description { state: \"default\" 0.0;<br/>",
   "      rel1 { relative: 0.0 0.0; offset: 0 0; /*to: \"XXX\";*/ }<br/>",
   "      rel2 { relative: 1.0 1.0; offset: -1 -1; /*to: \"XXX\";*/ }<br/>",
   "      align: 0.5 0.5;<br/>",
   "      fixed: 0 0;<br/>",
   "      min: 0 0;<br/>",
   "      visible: 1;<br/>",
   "      image.normal: \"logo.png\";<br/>",
   "      //aspect: 1 1;<br/>",
   "   }<br/>",
   "}"
};

#define TEMPLATE_PART_RECT_LINE_CNT 14

const char *TEMPLATE_PART_RECT[TEMPLATE_PART_RECT_LINE_CNT] =
{
   "   type: RECT;<br/>",
   "   scale: 1;<br/>",
   "   mouse_events: 1;<br/>",
   "   description { state: \"default\" 0.0;<br/>",
   "      rel1 { relative: 0.0 0.0; offset: 0 0; /*to: \"XXX\";*/ }<br/>",
   "      rel2 { relative: 1.0 1.0; offset: -1 -1; /*to: \"XXX\";*/ }<br/>",
   "      color: 255 255 255 255;<br/>",
   "      align: 0.5 0.5;<br/>",
   "      fixed: 0 0;<br/>",
   "      min: 0 0;<br/>",
   "      visible: 1;<br/>",
   "   }<br/>",
   "}"
};

#define TEMPLATE_PART_SWALLOW_LINE_CNT 13

const char *TEMPLATE_PART_SWALLOW[TEMPLATE_PART_SWALLOW_LINE_CNT] =
{
   "   type: SWALLOW;<br/>",
   "   scale: 1;<br/>",
   "   mouse_events: 1;<br/>",
   "   description { state: \"default\" 0.0;<br/>",
   "      rel1 { relative: 0.0 0.0; offset: 0 0; /*to: \"XXXX\";*/ }<br/>",
   "      rel2 { relative: 1.0 1.0; offset: -1 -1; /*to: \"XXX\";*/ }<br/>",
   "      align: 0.5 0.5;<br/>",
   "      fixed: 0 0;<br/>",
   "      min: 0 0;<br/>",
   "      visible: 1;<br/>",
   "   }<br/>",
   "}"
};

#define TEMPLATE_PART_SPACER_LINE_CNT 11

const char *TEMPLATE_PART_SPACER[TEMPLATE_PART_SPACER_LINE_CNT] =
{
   "   type: SPACER;<br/>",
   "   scale: 1;<br/>",
   "   description { state: \"default\" 0.0;<br/>",
   "      rel1 { relative: 0.0 0.0; offset: 0 0; /*to: \"XXX\";*/ }<br/>",
   "      rel2 { relative: 1.0 1.0; offset: -1 -1; /*to: \"XXX\";*/ }<br/>",
   "      align: 0.5 0.5;<br/>",
   "      fixed: 0 0;<br/>",
   "      min: 0 0;<br/>",
   "   }<br/>",
   "}"
};

#define TEMPLATE_PART_TEXT_LINE_CNT 24

const char *TEMPLATE_PART_TEXT[TEMPLATE_PART_TEXT_LINE_CNT] =
{
   "   type: TEXT;<br/>",
   "   scale: 1;<br/>",
   "   mouse_events: 1;<br/>",
   "   //effect: SHADOW;<br/>",
   "   description { state: \"default\" 0.0;<br/>",
   "      rel1 { relative: 0.0 0.0; offset: 0 0; /*to: \"XXX\";*/ }<br/>",
   "      rel2 { relative: 1.0 1.0; offset: -1 -1; /*to: \"XXX\";*/ }<br/>",
   "      align: 0.5 0.5;<br/>",
   "      fixed: 0 0;<br/>",
   "      min: 0 0;<br/>",
   "      color: 255 255 255 255;<br/>",
   "      //color2: 255 255 255 255;<br/>",
   "      //color3: 255 255 255 255;<br/>",
   "      visible: 1;<br/>",
   "      text {<br/>",
   "         size: 10;<br/>",
   "         font: \"Sans\";<br/>",
   "         text: \"Enventor\";<br/>",
   "         align: 0.5 0.5;<br/>",
   "         min: 0 0;<br/>",
   "      }<br/>",
   "   }<br/>",
   "}"
};

#define TEMPLATE_PART_TEXTBLOCK_LINE_CNT 18

const char *TEMPLATE_PART_TEXTBLOCK[TEMPLATE_PART_TEXTBLOCK_LINE_CNT] =
{
   "   type: TEXTBLOCK;<br/>",
   "   description { state: \"default\" 0.0;<br/>",
   "      rel1 { relative: 0.0 0.0; offset: 0 0; /*to: \"XXX\";*/ }<br/>",
   "      rel2 { relative: 1.0 1.0; offset: -1 -1; /*to: \"XXX\";*/ }<br/>",
   "      align: 0.5 0.5;<br/>",
   "      fixed: 0 0;<br/>",
   "      min: 0 0;<br/>",
   "      visible: 1;<br/>",
   "      text {<br/>",
   "         style: \"XXX\";<br/>",
   "         text: \"Enventor\";<br/>",
   "         align: 0.5 0.5;<br/>",
   "         min: 0 0;<br/>",
   "      }<br/>",
   "   }<br/>",
   "}"
};

#define TEMPLATE_DESC_LINE_CNT 10

const char *TEMPLATE_DESC[TEMPLATE_DESC_LINE_CNT] =
{
   "description { state: \"XXX\" 0.0;<br/>",
   "   //inherit: \"default\";<br/>",
   "   rel1 { relative: 0.0 0.0; offset: 0 0; /*to: \"XXX\";*/ }<br/>",
   "   rel2 { relative: 1.0 1.0; offset: -1 -1; /*to: \"XXX\";*/ }<br/>",
   "   align: 0.5 0.5;<br/>",
   "   fixed: 0 0;<br/>",
   "   min: 0 0;<br/>",
   "   visible: 1;<br/>",
   "   color: 255 255 255 255;<br/>",
   "}",
};

#define TEMPLATE_PROG_LINE_CNT 7

const char *TEMPLATE_PROG[TEMPLATE_PROG_LINE_CNT] =
{
   "program { name: \"XXX\";<br/>",
   "   //signal: \"XXX\";<br/>",
   "   //source: \"XXX\";<br/>",
   "   action: STATE_SET \"default\" 0.0;<br/>",
   "   target: \"XXX\";<br/>",
   "   //transition: LINEAR 1.0;<br/>",
   "}"
};

#define TEMPLATE_IMG_LINE_CNT 1

const char *TEMPLATE_IMG[TEMPLATE_IMG_LINE_CNT] =
{
   "image: \"logo.png\" COMP;"
};
