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
#include <string.h>
#include "html5_tokenizer.h"
#include "html5_parser.h"


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


gboolean parser_readToken(const gchar **html, Token *token, ParserState *state) {
    const gchar *previousPoint = *html;
    TagType type;
    
    if (state->specialState) {
        Token *current = &state->openTags[state->level-1];
        switch (state->specialState) {
            /*case SpecialState_InScript:
                tokenizer_skipScript(html);
                token->type = Token_None;
                return TRUE;*/
            case SpecialState_InRawText:
                tokenizer_skipRawText(html, current);
                break;
            case SpecialState_InRCData:
                tokenizer_skipRCData(html, current);
                break;
            default: break;
        }
        
        token->type = Token_SpecialText;
        token->data = previousPoint;
        token->dataLength = *html - previousPoint;
        state->specialState = SpecialState_Normal;
        return TRUE;
    }
    
    if (!tokenizer_readToken(html, token)) return FALSE;
    
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
                //state->specialState = SpecialState_InScript;
                tokenizer_skipScript(html);
                token->type = Token_None;
                return TRUE;
            } else if (type & TagType_HasRawText) {
                state->specialState = SpecialState_InRawText;
            } else if (type & TagType_HasRCData) {
                state->specialState = SpecialState_InRCData;
            } else if (token->tag == Tag_plaintext) {
                token->type = Token_Text;
                token->data = *html;
                token->dataLength = strlen(*html);
                (*html) += token->dataLength;
                return TRUE;
            }
            
            // Handle <li>, <dd> and <dt> specially
            if (type & TagType_ListItem) {
                if (token->tag == Tag_li) {
                    CLOSEONE(Tag_li, NULL, TagType_MatchNone, acceptListPhrasing, TRUE);
                } else {
                    CLOSEONE(Tag_dd, NULL, TagType_MatchNone, acceptListPhrasing, TRUE);
                    CLOSEONE(Tag_dt, NULL, TagType_MatchNone, acceptListPhrasing, TRUE);
                }
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



