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
#include <stdlib.h>
#include <string.h>
#include "tokenizer.h"

#include "template.h"

typedef struct {
    const gchar *name;
    size_t nameLength;
    
    const gchar *element;
    size_t elementLength;
} Marker;

struct Template_ {
    size_t markerCount;
    Marker *markers;
};


typedef struct {
    size_t level;
    Token *openTags;
    
    gboolean inBeginningOfElement : 1;
    gboolean inForm : 1;
    
    //enum { Normal = 0, InScript, InRawText, InRCData } specialState;
} ParserState;


typedef gboolean (*TagFilterFunction)(TokenTag tag, TagType type);
static gboolean acceptPhrasing(TokenTag tag, TagType type) { return type & TagType_Phrasing; }
static gboolean acceptListPhrasing(TokenTag tag, TagType type) { return type & TagType_Phrasing || tag == Tag_address || tag == Tag_div || tag == Tag_p; }
static gboolean limitToScope(TokenTag tag, TagType type) { return !(type & TagType_Scope); }


/**
 * Closes the current element.
 */
static void popTag(ParserState *state) {
    state->openTags = g_realloc(state->openTags, --state->level * sizeof(Token));
}

/**
 * Checks if the tag in "current" matches the given attributes.
 */
static gboolean compareTag(TokenTag tag, const gchar *tagName, TagType tagType,
                           const Token *current) {
    return (tagType & tokenizer_getTagType(current) || // Check type
            (tag != Tag_None && current->tag == tag) ||  // Tag identifier
            (tagName && // Tag name
                g_ascii_strncasecmp(current->data, tagName, current->dataLength) &&
                current->dataLength == strlen(tagName)));
}

/**
 * Closes an element by tag id/name or by type. If the tag has child elements
 * the child tag at the bottom of the tree is closed.
 *
 * @returns  TRUE if a new token has been emitted.
 */
static gboolean closeOne(const gchar **html, Token *token, ParserState *state,
                         const gchar *previousPoint,
                         TokenTag tag, const gchar *tagName,
                         TagType tagType,
                         TagFilterFunction canAutoClose) {
    if (state->level == 0) return FALSE;
    
    // Check that the tag (if any) is open in the current scope
    gboolean found = FALSE;
    for (size_t level = state->level; level-- > 0; ) {
        Token *current = &state->openTags[level];
        
        // Matching tag?
        if (compareTag(tag, tagName, tagType, current)) {
            found = TRUE;
            break;
        }
        
        // Check for block or scope tags and stop there
        TagType currentType = tokenizer_getTagType(current);
        if (!canAutoClose(current->tag, currentType)) return FALSE;
    } 
    
    if (!found) return FALSE;
    
    
    // Close the tag
    Token *current = &state->openTags[state->level-1];
    
    // Matching tag?
    //fprintf(stderr, "tag %d(%s) == %d(%s)? tt:%d --> ", tag, tokenizer_tagName[tag], current->tag, tokenizer_tagName[current->tag], tagType);
    if (compareTag(tag, tagName, tagType, current)) {
        fprintf(stderr, "emit end tag (%d %d %d)\n",
            tagType & tokenizer_getTagType(current),
            (tag != Tag_None && current->tag == tag),
            (tagName &&
                g_ascii_strncasecmp(current->data, tagName, current->dataLength) &&
                current->dataLength == strlen(tagName)));
        // Not exactly what the spec says, as I understand the spec
        // it shouldn't make any difference as long as formatting
        // information is unimportant.
        
        // Emit end tag
        token->type = Token_EndTag;
        token->tag = current->tag;
        
        popTag(state);
        return TRUE;
    }
    
    /*// Check for block or scope tags and stop there
    TagType currentType = tokenizer_getTagType(current);
    fprintf(stderr, "can auto close?\n");
    if (!canAutoClose(current->tag, currentType)) return FALSE;*/
    
    // Emit end tag (will repeat until there are no more tags to close)
    token->type = Token_EndTag;
    token->tag = current->tag;
    *html = previousPoint;
    fprintf(stderr, "emit end tag (implicit)\n");
    
    popTag(state);
    return TRUE;
}


#define CLOSEONE(tag, tagName, tagMatch, checkCloseFunc, keepToken) do { \
    if (closeOne(html, token, state, previousPoint, \
                 (tag), (tagName), (tagMatch), checkCloseFunc)) { \
        if (keepToken) *html = previousPoint; \
        return TRUE; \
    }\
} while (0);


static gboolean parse(const gchar **html, Token *token, ParserState *state) {
    const gchar *previousPoint = *html;
    TagType type;
    
    /*if (state->specialState) {
        Token *current = state->openTags[state->level-1];
        switch (state->specialState) {
            case InScript:
                tokenizer_skipScript(html);
                token->type = Token_None;
                return TRUE;
            case InRawText:
                tokenizer_skipScript(html, current);
                break;
            case InRCData:
                tokenizer_skipScript(html, current);
                break;
            default: break;
        }
        
        token->data = previousPoint;
        token->dataLength = *html - previousPoint;
        return TRUE;
    }*/
    
    if (!tokenizer_readTag(html, token)) return FALSE;
    
    switch (token->type) {
        case Token_StartTag:
            fprintf(stderr, "START TAG: [%.*s]\n", token->dataLength, token->data);
            state->inBeginningOfElement = FALSE;
            type = tokenizer_getTagType(token);
            
            if (type & TagType_Ignored) {
                token->type = Token_None;
                return TRUE;
            }
            
            // Ignore nested forms
            if (token->tag == Tag_form) {
                if (state->inForm) {
                    token->type = Token_None;
                    return TRUE;
                }
                state->inForm = 1;
            }
            
            // Auto-close <p> and <h#>
            if (type & TagType_EndsParagraph) { CLOSEONE(Tag_p, NULL, TagType_MatchNone, acceptPhrasing, TRUE); }
            if (type & TagType_Heading) { CLOSEONE(Tag_None, NULL, TagType_Heading, acceptPhrasing, TRUE); }
            
            // Check for void tags
            if (type & TagType_Void) {
                token->type = Token_SelfClosingTag;
                return TRUE;
            }
            
            // Handle tags with special content
            // TODO handle different insertion modes
            if (type & TagType_HasScript) {
                //state->specialState = InScript;
                tokenizer_skipScript(html);
                token->type = Token_None;
                return TRUE;
            } else if (type & TagType_HasRawText) {
                //state->specialState = InRawText;
                tokenizer_skipRawText(html, token);
                token->type = Token_None;
                return TRUE;
            } else if (type & TagType_HasRCData) {
                //state->specialState = InRCData;
                tokenizer_skipRCData(html, token);
                token->type = Token_None;
                return TRUE;
            } else if (token->tag == Tag_plaintext) {
                token->type = Token_Text;
                token->data = *html;
                token->dataLength = strlen(*html);
                (*html) += token->dataLength;
                return TRUE;
            }
            
            // Handle <li>, <dd> and <dt> specially
            if (type & TagType_ListItem) {
                CLOSEONE(token->tag, NULL, TagType_MatchNone, acceptListPhrasing, TRUE);
            }
            
            // "Formatting" elements like <b> are not handled here because
            // we're not interested in knowing style properties of the page.
            
            // Open the tag
            state->openTags = g_realloc(state->openTags, ++state->level * sizeof(Token));
            memcpy(&state->openTags[state->level-1], token, sizeof(Token));
            
            state->inBeginningOfElement = TRUE;
            break;
        case Token_EndTag:
            fprintf(stderr, "END TAG: [%.*s]\n", token->dataLength, token->data);
            state->inBeginningOfElement = FALSE;
            type = tokenizer_getTagType(token);
            
            // <h#> tags can be closed with other <h#> tags
            if (type & TagType_Heading) { CLOSEONE(Tag_None, NULL, TagType_Heading, acceptPhrasing, FALSE); }
            
            CLOSEONE(token->tag, NULL, TagType_MatchNone, limitToScope, FALSE);
            
            // No matching tag was found
            token->type = Token_None;
            break;
        case Token_SelfClosingTag:
        case Token_Text:
            state->inBeginningOfElement = FALSE;
            break;
        case Token_None: return FALSE;
        default: break;
    }
    
    return TRUE;
}


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
    while (*end && end[0] != '-' && end[1] != '-') end++;
    if (!*end) return NULL;
    
    // Remove trailing spaces
    end--;
    while (g_ascii_isspace(*end)) end--;
    
    *length = end - start;
    return start;
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
    
    while (parse(&html, &token, &state)) {
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
        
        
        size_t nameLength;
        const gchar *name = getSpecialComment(&token, &state, &nameLength);
        if (name) {
            // TODO add a marker for the current element
            fprintf(stderr, "found special comment: [%.*s]\n", nameLength, name);
        }
    }
    
    return tem;
}


void template_free(Template *tem) {
    g_free(tem);
}


gchar *template_updatePage(Template *tem, const gchar *pageHTML) {
    // TODO
    return NULL;
}


