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


#include <string.h>

#include "tokenizer.h"


// All HTML5 tags as strings. Generated with "make html5enum"
const char *tokenizer_tagName[] = {
    "a", "abbr", "acronym", "address", "applet", "area", "article", "aside",
    "audio", "b", "base", "basefont", "bdo", "bgsound", "big", "blink",
    "blockquote", "body", "br", "button", "canvas", "caption", "center",
    "cite", "code", "col", "colgroup", "command", "datalist", "dd", "del",
    "details", "dfn", "dir", "div", "dl", "dt", "em", "embed", "fieldset",
    "figcaption", "figure", "font", "footer", "form", "frame", "frameset",
    "h1", "h2", "h3", "h4", "h5", "h6", "head", "header", "hgroup", "hr",
    "html", "i", "iframe", "img", "input", "ins", "isindex", "kbd", "keygen",
    "label", "legend", "li", "link", "listing", "map", "mark", "marquee",
    "menu", "meta", "meter", "multicol", "nav", "nextid", "nobr", "noembed",
    "noframes", "noscript", "object", "ol", "optgroup", "option", "output",
    "p", "param", "plaintext", "pre", "progress", "q", "rb", "rp", "rt",
    "ruby", "s", "samp", "script", "section", "select", "small", "source",
    "spacer", "span", "strike", "strong", "style", "sub", "summary", "sup",
    "table", "tbody", "td", "textarea", "tfoot", "th", "thead", "time",
    "title", "tr", "tt", "u", "ul", "var", "wbr", "video", "xmp", NULL,
    // aliases: <image> = <img>
};

#define NONE (0)
#define INLINE (TagType_Phrasing)
#define BLOCK (TagType_EndsParagraph)
#define HEADING (TagType_Heading | TagType_EndsParagraph)
#define VOID (TagType_Void)
#define IGNORED (TagType_Ignored)

#define DEFAULT (TagType_Phrasing)

const TagType tokenizer_tagType[] = {
    // <a>
    INLINE,
    // <abbr>
    INLINE,
    // <acronym>(d)
    DEFAULT,
    // <address>
    BLOCK,
    // <area>
    VOID,
    // <applet>(d)
    DEFAULT | TagType_Scope,
    // <article>
    BLOCK,
    // <aside>
    BLOCK,
    // <audio>
    INLINE,
    // <b>
    INLINE,
    // <base>
    VOID,
    // <basefont>(d)
    VOID,
    // <bdo>
    INLINE,
    // <bgsound>(d)
    VOID,
    // <big>(d)
    INLINE,
    // <blink>(d)
    INLINE,
    // <blockquote>
    BLOCK,
    // <body>
    NONE,
    // <br>
    VOID,
    // <button>
    INLINE | TagType_Scope,
    // <canvas>
    INLINE,
    // <caption>
    BLOCK | TagType_Scope,
    // <center>(d)
    BLOCK, //  = <div align="center">
    // <cite>
    INLINE,
    // <code>
    INLINE,
    // <col>
    VOID,
    // <colgroup>
    0,
    // <command>
    VOID,
    // <datalist>
    INLINE,
    // <dd>
    TagType_ListItem,
    // <del>
    INLINE,
    // <details>
    BLOCK,
    // <dfn>
    INLINE,
    // <dir>(d)
    BLOCK,
    // <div>
    BLOCK,
    // <dl>
    BLOCK,
    // <dt>
    TagType_ListItem,
    // <em>
    INLINE,
    // <embed>
    VOID,
    // <fieldset>
    BLOCK,
    // <figcaption>
    0,
    // <figure>
    BLOCK,
    // <font>(d)
    INLINE,
    // <footer>
    BLOCK,
    // <form>
    BLOCK,
    // <frame>(d)
    IGNORED,
    // <frameset>(d)
    IGNORED,
    // <h1>
    HEADING,
    // <h2>
    HEADING,
    // <h3>
    HEADING,
    // <h4>
    HEADING,
    // <h5>
    HEADING,
    // <h6>
    HEADING,
    // <head>
    0,
    // <header>
    BLOCK,
    // <hgroup>
    BLOCK,
    // <hr>
    VOID,
    // <html>
    TagType_Scope,
    // <i>
    INLINE,
    // <iframe>
    INLINE | TagType_HasRawText,
    // <img>
    VOID,
    // <input>
    VOID,
    // <ins>
    INLINE,
    // <isindex>(d)
    VOID, // not supported at all though
    // <kbd>
    INLINE,
    // <keygen>
    VOID,
    // <label>
    INLINE,
    // <legend>
    0,
    // <li>
    TagType_ListItem,
    // <link>
    VOID,
    // <listing>(d)
    BLOCK, // can't find it specified anywhere...
    // <map>
    INLINE,
    // <mark>
    INLINE,
    // <marquee>
    INLINE | TagType_Scope,
    // <menu>
    BLOCK,
    // <meta>
    VOID,
    // <meter>
    INLINE,
    // <multicol>(d)
    DEFAULT,
    // <nav>
    BLOCK,
    // <nextid>(d)
    DEFAULT, // can't find it specified anywhere...
    // <nobr>(d)
    INLINE,
    // <noembed>(d)
    DEFAULT | TagType_HasRawText,
    // <noframes>(d)
    DEFAULT | TagType_HasRawText,
    // <noscript>
    INLINE,
    // <object>
    INLINE | TagType_Scope,
    // <ol>
    BLOCK,
    // <optgroup>
    0,
    // <option>
    0,
    // <output>
    INLINE,
    // <p>
    BLOCK,
    // <param>
    VOID,
    // <plaintext>(d)
    DEFAULT, // Handled specially
    // <pre>
    BLOCK,
    // <progress>
    INLINE,
    // <q>
    INLINE,
    // <rb>(d)
    DEFAULT,
    // <rp>
    0,
    // <rt>
    0,
    // <ruby>
    INLINE,
    // <s>(d)
    INLINE,
    // <samp>
    INLINE,
    // <script>
    INLINE | TagType_HasScript,
    // <section>
    BLOCK,
    // <select>
    INLINE,
    // <small>
    INLINE,
    // <source>
    VOID,
    // <spacer>(d)
    DEFAULT,
    // <span>
    INLINE,
    // <strike>(d)
    INLINE,
    // <strong>
    INLINE,
    // <style>
    TagType_HasRawText,
    // <sub>
    INLINE,
    // <summary>
    0,
    // <sup>
    INLINE,
    // <table>
    BLOCK | TagType_Scope,
    // <tbody>
    0,
    // <td>
    TagType_Scope,
    // <textarea>
    INLINE | TagType_HasRCData,
    // <tfoot>
    0,
    // <th>
    TagType_Scope,
    // <thead>
    0,
    // <time>
    INLINE,
    // <title>
    TagType_HasRCData,
    // <tr>
    0,
    // <tt>(d)
    INLINE,
    // <u>(d)
    INLINE,
    // <ul>
    BLOCK,
    // <var>
    INLINE,
    // <video>
    INLINE,
    // <wbr>
    INLINE,
    // <xmp>(d)
    INLINE | TagType_HasRawText,
};

const TagType UnknownTagType = DEFAULT;


#define STEP(html) do { (*html)++; if (!**html) return FALSE; } while (0)
static gboolean skipAttributes(const gchar **html) {
    if (!**html) return FALSE;
    
    beforeAttribute: for (;;) {
        // Skip whitespace and self-closing "/" marks
        while (strchr("\t\v\n\r /", **html)) STEP(html);
        if (**html == '>') return TRUE;
        
        // Read attribute name
        for (;;) {
            STEP(html);
            if (strchr("\t\v\n\r /", **html)) goto beforeAttribute;
            if (**html == '>') return TRUE;
            if (**html == '=') break;
        }
        
        // Read attribute value
        STEP(html);
        gboolean doubleQuoted = (**html == '"');
        gboolean singleQuoted = (**html == '\'');
        if (**html == '>') return TRUE;
        for (;;) {
            STEP(html);
            if (doubleQuoted) { if (**html == '"') break; }
            else if (singleQuoted) { if (**html == '\'') break; }
            else if (strchr("\t\v\n\r /", **html)) goto beforeAttribute;
            else if (**html == '>') return TRUE;
        }
        STEP(html); // Skip quote character
    }
}


gboolean tokenizer_readTag(const gchar **html, Token *token) {
    token->tag = Tag_None;
    token->data = NULL;
    token->dataLength = 0;
    
    // Handle end
    if (**html == '\0') {
        token->type = Token_None;
        return FALSE;
    }
    
    // Handle space
    if (g_ascii_isspace(**html)) {
        token->type = Token_Space;
        token->data = *html;
        while (g_ascii_isspace(**html)) { (*html)++; }
        token->dataLength = *html - token->data;
        return TRUE;
    }
    
    // Handle text
    if (**html != '<') {
        token->type = Token_Text;
        token->data = *html;
        while (**html && **html != '<') { (*html)++; }
        token->dataLength = *html - token->data;
        return TRUE;
    }
    
    // Handle "<"
    STEP(html);
    
    // Handle comments
    if (**html == '!' || **html == '?') {
        // TODO handle [CDATA[ in "in foreign content" insertion mode
        token->type = Token_Comment;
        token->data = *html;
        while (**html && **html != '>') { (*html)++; }
        token->dataLength = *html - token->data;
        (*html)++;
        return TRUE;
    }
    
    // Handle tag
    gboolean endTag = (**html == '/');
    if (endTag) (*html)++;
    
    // Check for bad tags
    if (**html == '>') {
        (*html)++;
        token->type = Token_None;
        return TRUE;
    } else if (!endTag && !g_ascii_isalpha(**html)) {
        token->type = Token_Text;
        token->data = &(*html)[-1];
        token->dataLength = 1;
        return TRUE;
    }
    
    // Read tag name
    token->data = *html;
    while (**html && !strchr("\t\v\n\r />", **html)) { (*html)++; }
    token->dataLength = *html - token->data;
    
    gchar *tagName = g_ascii_strdown(token->data, token->dataLength);
    for (TagType t = 0; t < TagCount; t++) {
        if (!strcmp(tagName, tokenizer_tagName[t])) {
            token->tag = t;
            break;
        }
    }
    
    if (token->tag == Tag_None && strcmp(tagName, "image")) {
        token->tag = Tag_img;
    }
    g_free(tagName);
    
    // Skip attributes, etc.
    if (!skipAttributes(html)) {
        token->type = Token_None;
        return FALSE;
    }
    
    // Detect tag type
    gboolean selfClosing = ((*html)[-1] == '/');
    if (endTag) token->type = Token_EndTag;
    else if (selfClosing) token->type = Token_SelfClosingTag;
    else token->type = Token_StartTag;
    
    //if (**html == '>') (*html)++; // always true?
    (*html)++; // Skip ">"
    return TRUE;
}


gboolean tokenizer_skipScript(const gchar **html) {
    // TODO
    // almost identical to skipRCData, except that it handles
    // <!-- --> comments (but not <! > or <!-- > comments)
    return TRUE;
}

gboolean tokenizer_skipRawText(const gchar **html, const Token *startTag) {
    // TODO
    // almost identical to skipRCData
    return TRUE;
}


gboolean tokenizer_skipRCData(const gchar **html, const Token *startTag) {
    // Skips "RCData" (<textarea> contents)
    for (; **html; (*html)++) {
        if (*(*html)++ != '<') continue;
        if (*(*html)++ != '/') continue;
        
        if (strncmp(*html, startTag->data, startTag->dataLength) != 0) continue;
        *html += startTag->dataLength;
        
        if (**html && strchr("\t\v\n\r />", **html)) {
            // The correct end tag
            if (!skipAttributes(html)) return FALSE;
            
            (*html)++; // Skip ">"
            return TRUE;
        }
    }
    return FALSE;
}


