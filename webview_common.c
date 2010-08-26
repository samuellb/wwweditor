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

#include "webview_private.h"

#include <stdarg.h>


/**
 * Reads a line from a string and advances the string pointer to the next
 * line.
 */
gchar *pop_item(gchar **str) {
    gchar *start = *str;
    gchar c;
    
    while ((c = **str) != '\0') {
        if (c == '\n') {
            *(*str)++ = '\0';
            return start;
        }
        (*str)++;
    }
    
    return start;
}


WebViewElementInfo *webview_private_createElementInfo(const gchar *jsString) {
    WebViewElementInfo *info = g_malloc(sizeof(WebViewElementInfo));
    
    gchar *str = g_strdup(jsString);
    
    info->tagName = pop_item(&str);
    info->styles = pop_item(&str);
    info->linkHref = pop_item(&str);
    info->title = pop_item(&str);
    
    return info;
}


void webview_private_freeElementInfo(WebViewElementInfo *info) {
    g_free(info->tagName); // frees all strings
    g_free(info);
}


void webview_executeFormattedScript(WebView *webview,
                                    const char *format, ...) {
    va_list args;
    va_start(args, format);
    
    gchar *script = g_strdup_vprintf(format, args);
    
    webview_executeScript(webview, script);
    
    g_free(script);
    va_end(args);
}




