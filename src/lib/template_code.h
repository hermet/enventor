#define TEMPLATE_GROUP_LINE_CNT 27

const char *TEMPLATE_GROUP[TEMPLATE_GROUP_LINE_CNT] =
{
   "group { \"XXX\";<br/>",
   "   parts {<br/>",
   "      image { \"XXX\";<br/>",
   "         scale: 1;<br/>",
   "         desc { \"default\";<br/>",
   "            rel1 { relative: 0.25 0.25; offset: 0 0; /*to: \"XXX\";*/ }<br/>",
   "            rel2 { relative: 0.75 0.75; offset: -1 -1; /*to: \"XXX\";*/ }<br/>",
   "            align: 0.5 0.5;<br/>",
   "            fixed: 0 0;<br/>",
   "            min: 0 0;<br/>",
   "            visible: 1;<br/>",
   "            /* TODO: Please replace embedded image files to your application image files. */<br/>",
   "            image.normal: \"ENVENTOR_EMBEDDED_LOGO.png\";<br/>",
   "            //aspect: 1 1;<br/>",
   "         }<br/>",
   "      }<br/>",
   "   }<br/>",
   "   programs {<br/>",
   "      program { \"XXX\";<br/>",
   "         //signal: \"XXX\";<br/>",
   "         //source: \"XXX\";<br/>",
   "         action: STATE_SET \"default\";<br/>",
   "         target: \"XXX\";<br/>",
   "         //transition: LINEAR 1.0;<br/>",
   "      }<br/>",
   "   }<br/>",
   "}"
};

#define TEMPLATE_PART_TALE_LINE_CNT 2

const char *TEMPLATE_PART_TALE[TEMPLATE_PART_TALE_LINE_CNT] =
{
   "   }<br/>",
   "}"
};

#define TEMPLATE_PART_IMAGE_LINE_CNT 9

const char *TEMPLATE_PART_IMAGE[TEMPLATE_PART_IMAGE_LINE_CNT] =
{
   "   scale: 1;<br/>",
   "   desc { \"default\";<br/>",
   "      align: 0.5 0.5;<br/>",
   "      fixed: 0 0;<br/>",
   "      min: 0 0;<br/>",
   "      visible: 1;<br/>",
   "      /* TODO: Please replace embedded image files to your application image files. */<br/>",
   "      image.normal: \"ENVENTOR_EMBEDDED_LOGO.png\";<br/>",
   "      //aspect: 1 1;<br/>"
};

#define TEMPLATE_PART_RECT_LINE_CNT 7

const char *TEMPLATE_PART_RECT[TEMPLATE_PART_RECT_LINE_CNT] =
{
   "   scale: 1;<br/>",
   "   desc { \"default\";<br/>",
   "      color: 0 136 170 255;<br/>",
   "      align: 0.5 0.5;<br/>",
   "      fixed: 0 0;<br/>",
   "      min: 0 0;<br/>",
   "      visible: 1;<br/>"
};

#define TEMPLATE_PART_SWALLOW_LINE_CNT 6

const char *TEMPLATE_PART_SWALLOW[TEMPLATE_PART_SWALLOW_LINE_CNT] =
{
   "   scale: 1;<br/>",
   "   desc { \"default\";<br/>",
   "      align: 0.5 0.5;<br/>",
   "      fixed: 0 0;<br/>",
   "      min: 0 0;<br/>",
   "      visible: 1;<br/>"
};

#define TEMPLATE_PART_SPACER_LINE_CNT 5

const char *TEMPLATE_PART_SPACER[TEMPLATE_PART_SPACER_LINE_CNT] =
{
   "   scale: 1;<br/>",
   "   desc { \"default\";<br/>",
   "      align: 0.5 0.5;<br/>",
   "      fixed: 0 0;<br/>",
   "      min: 0 0;<br/>"
};

#define TEMPLATE_PART_TEXT_LINE_CNT 17

const char *TEMPLATE_PART_TEXT[TEMPLATE_PART_TEXT_LINE_CNT] =
{
   "   scale: 1;<br/>",
   "   effect: FAR_SOFT_SHADOW;<br/>",
   "   desc { \"default\";<br/>",
   "      align: 0.5 0.5;<br/>",
   "      fixed: 0 0;<br/>",
   "      min: 0 0;<br/>",
   "      color: 0 136 170 255;<br/>",
   "      color2: 0 136 170 50;<br/>",
   "      color3: 0 136 170 25;<br/>",
   "      visible: 1;<br/>",
   "      text {<br/>",
   "         size: 50;<br/>",
   "         font: \"Sans\";<br/>",
   "         text: \"TEXT\";<br/>",
   "         align: 0.5 0.5;<br/>",
   "         min: 0 0;<br/>",
   "      }<br/>"
};

#define TEMPLATE_PART_TEXTBLOCK_LINE_CNT 7

const char *TEMPLATE_PART_TEXTBLOCK[TEMPLATE_PART_TEXTBLOCK_LINE_CNT] =
{
   "   scale: 1;<br/>",
   "   desc { \"default\";<br/>",
   "      align: 0.5 0.5;<br/>",
   "      fixed: 0 0;<br/>",
   "      min: 0 0;<br/>",
   "      visible: 1;<br/>",
   "      text.text: \"TEXTBLOCK\";<br/>"
};

#define TEMPLATE_DESC_LINE_CNT 10

const char *TEMPLATE_DESC[TEMPLATE_DESC_LINE_CNT] =
{
   "desc { \"XXX\";<br/>",
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
   "program { \"XXX\";<br/>",
   "   //signal: \"XXX\";<br/>",
   "   //source: \"XXX\";<br/>",
   "   action: STATE_SET \"default\";<br/>",
   "   target: \"XXX\";<br/>",
   "   //transition: LINEAR 1.0;<br/>",
   "}"
};

#define TEMPLATE_IMG_LINE_CNT 2

const char *TEMPLATE_IMG[TEMPLATE_IMG_LINE_CNT] =
{
   "/* TODO: Please replace embedded image files to your application image files. */<br/>",
   "image: \"ENVENTOR_EMBEDDED_LOGO.png\" COMP;<br/>"
};

#define TEMPLATE_IMG_BLOCK_LINE_CNT 4

const char *TEMPLATE_IMG_BLOCK[TEMPLATE_IMG_BLOCK_LINE_CNT] =
{
   "/* TODO: Please replace embedded image files to your application image files. */<br/>",
   "images {<br/>",
   "   image: \"ENVENTOR_EMBEDDED_LOGO.png\" COMP;<br/>",
   "}<br/>"
};

#define TEMPLATE_TEXTBLOCK_STYLE_LINE_CNT 5

const char *TEMPLATE_TEXTBLOCK_STYLE_BLOCK[TEMPLATE_TEXTBLOCK_STYLE_LINE_CNT] =
{
   "styles {<br/>",
   "   style { \"%s\";<br/>",
   "      base: \"font=\"Sans\" font_size=30 text_class=entry color=#0088AA style=shadow,bottom shadow_color=#00000080 valign=0.5 ellipsis=1.0 wrap=none align=center\";<br/>",
   "   }<br/>",
   "}<br/>"
};

