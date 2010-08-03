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

#include <stdio.h> // TODO remove (used for debug code)
#include "html5_parser.h"

#include "template.h"

typedef struct {
    const gchar *name;
    size_t nameLength;
    
    const gchar *startTag;
    
    const gchar *content;
    size_t contentLength;
} Marker;

struct Template_ {
    size_t markerCount;
    Marker *markers;
};


static const gchar *getSpecialComment(const Token *token,
                                      const ParserState *state,
                                      size_t *length) {
    // Look for comments that immediately follow a start tag
    if (token->type != Token_Comment ||
        !state->inBeginningOfElement) return NULL;
    
    if (!g_str_has_prefix(token->data, "!--")) return NULL;
    
    
    // Read " @ "
    const gchar *start = token->data + 3;
    while (*start && g_ascii_isspace(*start)) start++;
    if (*start != '@') return NULL;
    start++;
    while (*start && g_ascii_isspace(*start)) start++;
    if (!*start) return NULL;
    
    // Read name
    const gchar *end = start;
    while (*end && (end[0] != '-' || end[1] != '-')) end++;
    if (!*end) return NULL;
    
    // Remove trailing spaces
    end--;
    while (g_ascii_isspace(*end)) end--;
    
    *length = end - start + 1;
    return start;
}


static Marker *addMarker(Template *tem) {
    tem->markers = g_realloc(tem->markers, ++tem->markerCount * sizeof(Marker));
    return &tem->markers[tem->markerCount-1];
}


static gboolean isTagClosed(ParserState *state, Marker *marker) {
    for (size_t l = state->level; l-- > 0; ) {
        if (state->openTags[l].data == marker->startTag) return FALSE;
    }
    return TRUE;
}


/**
 * Parses a template document. The document can contain any number of
 * editable elements. An element becomes editable if it the start tag is
 * followed by a special comment, for example:
 *
 *     <div>
 *       <!--@Page contents-->
 *       default contents here...
 *     </div>
 */
Template *template_parseFromString(const gchar *templateHTML) {
    Template *tem = g_malloc(sizeof(Template));
    tem->markerCount = 0;
    tem->markers = NULL;
    
    ParserState state = { 0, NULL, 0, 0 };
    Token token;
    const gchar *html = templateHTML;
    const gchar *prevTokenEnd = html;
    const gchar *prevTagEnd = NULL;
    Marker *marker = NULL;
    
    while (parser_readToken(&html, &token, &state)) {
        fprintf(stderr, "got token: (%d:%d) [%s%s]\n", token.type, token.tag,
            (token.type == Token_EndTag ? "/" : ""),
            (token.type == Token_StartTag || token.type == Token_EndTag || token.type == Token_SelfClosingTag ? tokenizer_tagName[token.tag] : "-"));
        if (token.type == Token_StartTag || token.type == Token_EndTag || token.type == Token_SelfClosingTag) {
            printf("<%s {%s} >\n", (token.type == Token_EndTag ? "/" : ""), tokenizer_tagName[token.tag]);
        } else if (token.type == Token_Comment) {
            printf("    <!-- {%.*s} -->\n", token.dataLength, token.data);
        } else if (token.type == Token_Text) {
            printf("    %d {%.*s}\n", token.type, token.dataLength, token.data);
        }
        
        if (!marker) {
            // Look for <!--@section--> comments
            size_t nameLength;
            const gchar *name = getSpecialComment(&token, &state, &nameLength);
            if (name) {
                fprintf(stderr, "found special comment: [%.*s]\n", nameLength, name);
                
                // Add marker
                marker = addMarker(tem);
                marker->name = name;
                marker->nameLength = nameLength;
                marker->startTag = state.openTags[state.level-1].data;
                marker->content = prevTagEnd;
                marker->contentLength = 0;
            }
        } else if (isTagClosed(&state, marker)) {
            // Finish the marker
            marker->contentLength = prevTokenEnd - marker->content;
            fprintf(stderr, "tag closed! >%.*s<\n", marker->nameLength, marker->name);
            fprintf(stderr, "contents %.*s\n", marker->contentLength, marker->content);
            marker = NULL;
        }
        
        prevTokenEnd = html;
        if (token.type == Token_StartTag) {
            prevTagEnd = html;
        }
    }
    
    return tem;
}


void template_free(Template *tem) {
    g_free(tem->markers);
    g_free(tem);
}


gchar *template_updatePage(Template *tem, const gchar *pageHTML) {
    // TODO
    return NULL;
}


