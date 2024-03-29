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

#ifndef WEBVIEW_H
#define WEBVIEW_H

#include <gtk/gtk.h>

typedef struct WebView_ WebView;

typedef struct {
    char *tagName;
    char *styles;
    
    char *linkHref;
    char *title;
} WebViewElementInfo;
typedef void (*WebViewNotifyFunction)(WebView *view, const WebViewElementInfo *info);
typedef void (*WebViewTitleChangeFunction)(WebView *view, const gchar *title);

WebView *webview_new(WebViewNotifyFunction notifyFunction,
                     WebViewTitleChangeFunction titleChangeFunction);
void webview_free(WebView *webview);

GtkWidget *webview_getWidget(WebView *webview);

void webview_load(WebView *webview, const gchar *url, const gchar *content,
                  gboolean wholePageEditable);
gchar *webview_getHTML(WebView *webview);
void webview_setTitle(WebView *webview, const gchar *title);

void webview_executeScript(WebView *webview, const gchar *script);
void webview_executeFormattedScript(WebView *webview,
                                    const char *format, ...);
gchar *webview_executeExpression(WebView *webview, const gchar *expr);

#endif


