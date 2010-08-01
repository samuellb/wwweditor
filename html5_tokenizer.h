/*

  Copyright (c) 2010 Samuel Lidén Borell <samuel@slbdata.se>

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.

*/

#ifndef HTML5_TOKENIZER_H
#define HTML5_TOKENIZER_H

#include <glib.h>


typedef enum {
    Token_None,
    Token_Space,
    Token_SelfClosingTag,
    Token_StartTag,
    Token_EndTag,
    Token_Comment, // includes doctypes
    Token_Text,
} TokenType;

// All HTML5 tags. Generated with "make html5enum"
typedef enum {
    Tag_None = -1,
    Tag_a, Tag_abbr, Tag_acronym, Tag_address, Tag_applet, Tag_area,
    Tag_article, Tag_aside, Tag_audio, Tag_b, Tag_base, Tag_basefont,
    Tag_bdo, Tag_bgsound, Tag_big, Tag_blink, Tag_blockquote, Tag_body,
    Tag_br, Tag_button, Tag_canvas, Tag_caption, Tag_center, Tag_cite,
    Tag_code, Tag_col, Tag_colgroup, Tag_command, Tag_datalist, Tag_dd,
    Tag_del, Tag_details, Tag_dfn, Tag_dir, Tag_div, Tag_dl, Tag_dt, Tag_em,
    Tag_embed, Tag_fieldset, Tag_figcaption, Tag_figure, Tag_font, Tag_footer,
    Tag_form, Tag_frame, Tag_frameset, Tag_h1, Tag_h2, Tag_h3, Tag_h4,
    Tag_h5, Tag_h6, Tag_head, Tag_header, Tag_hgroup, Tag_hr, Tag_html,
    Tag_i, Tag_iframe, Tag_img, Tag_input, Tag_ins, Tag_isindex, Tag_kbd,
    Tag_keygen, Tag_label, Tag_legend, Tag_li, Tag_link, Tag_listing, Tag_map,
    Tag_mark, Tag_marquee, Tag_menu, Tag_meta, Tag_meter, Tag_multicol,
    Tag_nav, Tag_nextid, Tag_nobr, Tag_noembed, Tag_noframes, Tag_noscript,
    Tag_object, Tag_ol, Tag_optgroup, Tag_option, Tag_output, Tag_p,
    Tag_param, Tag_plaintext, Tag_pre, Tag_progress, Tag_q, Tag_rb, Tag_rp,
    Tag_rt, Tag_ruby, Tag_s, Tag_samp, Tag_script, Tag_section, Tag_select,
    Tag_small, Tag_source, Tag_spacer, Tag_span, Tag_strike, Tag_strong,
    Tag_style, Tag_sub, Tag_summary, Tag_sup, Tag_table, Tag_tbody, Tag_td,
    Tag_textarea, Tag_tfoot, Tag_th, Tag_thead, Tag_time, Tag_title, Tag_tr,
    Tag_tt, Tag_u, Tag_ul, Tag_var, Tag_wbr, Tag_video, Tag_xmp,
    TagCount,
} TokenTag;

typedef enum {
    TagType_EndsParagraph =  0x10,
    TagType_Heading =        0x20,
    TagType_Void =           0x40,
    TagType_Phrasing =       0x80,
    TagType_Scope =         0x100,
    TagType_ListItem =      0x200,
    TagType_Ignored =       0x400,
    
    TagType_HasScript =       0x1,
    TagType_HasRCData =       0x2,
    TagType_HasRawText =      0x4,
    
    TagType_MatchNone =         0,
} TagType;

extern const TagType UnknownTagType;

extern const char* tokenizer_tagName[];
extern const TagType tokenizer_tagType[];


typedef struct {
    TokenType type;
    TokenTag tag;
    
    const gchar *data;
    size_t dataLength;
} Token;


#define tokenizer_getTagType(token) ((token)->tag != Tag_None ? \
    tokenizer_tagType[(token)->tag] : UnknownTagType)

gboolean tokenizer_readToken(const gchar **html, Token *token);
gboolean tokenizer_skipScript(const gchar **html);
gboolean tokenizer_skipRawText(const gchar **html, const Token *startTag);
gboolean tokenizer_skipRCData(const gchar **html, const Token *startTag);


#endif

