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

#include "webview.h"
#include "webview_private.h"

#include <webkit/webkit.h>

struct WebView_ {
    WebViewCommon common;
    
    GtkWidget *widget;
    guint link_blocker;
};


/**
 * Triggered when the document load status changes. Used to detect when the
 * document has finished loading.
 */
static void load_status_notify(GObject *gobject, GParamSpec *pspec, gpointer user_data) {
    WebKitLoadStatus status;
    g_object_get(gobject, "load-status", &status,  NULL);
    
    if (status != WEBKIT_LOAD_FINISHED) return;
    
    WebView *webview = (WebView*)user_data;
    GObject *settings = G_OBJECT(webkit_web_view_get_settings(WEBKIT_WEB_VIEW(webview->widget)));
    
    // Initialize the editor script
    g_object_set(settings, "enable-scripts", TRUE,  NULL);
    webview_private_initEditorScript(webview);
}


/**
 * Triggered on changes to the "window.status" property. Used for receiving
 * messages from Javascript code.
 */
static void status_changed(WebKitWebView *widget, const gchar *text, gpointer user_data) {
    if (!text) return;
    
    WebViewElementInfo *info = webview_private_createElementInfo(text);
    
    WebView *webview = (WebView*)user_data;
    webview->common.notifyFunction(webview, info);
    
    webview_private_freeElementInfo(info);
}


/**
 * Triggered when the document title is changed
 */
static void title_notify(GObject *gobject, GParamSpec *pspec, gpointer user_data) {
    gchar *title;
    g_object_get(gobject, "title", &title,  NULL);
    
    WebView *webview = (WebView*)user_data;
    webview->common.titleChangeFunction(webview, title ? title : "");
    
    g_free(title);
}


WebView *webview_new(WebViewNotifyFunction notifyFunction,
                     WebViewTitleChangeFunction titleChangeFunction) {
    WebView *webview = g_malloc(sizeof(WebView));
    webview->common.notifyFunction = notifyFunction;
    webview->common.titleChangeFunction = titleChangeFunction;
    
    // Create an editable WebKit view
    GtkWidget *widget = webkit_web_view_new();
    webview->widget = widget;
    webview->link_blocker = 0;
    
    // Prepare settings and values
    GObject *settings = G_OBJECT(webkit_web_view_get_settings(WEBKIT_WEB_VIEW(widget)));
    
    g_object_set(settings,
                 // Disable plugins, etc.
                 "enable-java-applet", FALSE,
                 "enable-plugins", FALSE,
                 "enable-site-specific-quirks", FALSE,
                 // Enable spell checking
                 // TODO check spelling of existing content
                 "enable-spell-checking", FALSE,
                 // Handle popup menus
                 "enable-default-context-menu", FALSE,
                 NULL);
    
    // Use "user-stylesheet-uri" for debugging?
    
    // Install handlers
    g_signal_connect(widget, "notify::load-status", G_CALLBACK(load_status_notify), webview);
    g_signal_connect(widget, "status-bar-text-changed", G_CALLBACK(status_changed), webview);
    g_signal_connect(widget, "notify::title", G_CALLBACK(title_notify), webview);
    
    // TODO release settings object?
    
    return webview;
}


void webview_free(WebView *webview) {
    g_free(webview);
}


GtkWidget *webview_getWidget(WebView *webview) {
    return webview->widget;
}


static gboolean block_navigation(WebKitWebView *widget, WebKitWebFrame *frame,
                                 WebKitNetworkRequest *request,
                                 WebKitWebNavigationAction *navigation_action,
                                 WebKitWebPolicyDecision *policy_decision,
                                 gpointer user_data) {
    webkit_web_policy_decision_ignore(policy_decision);
    return TRUE;
}


void webview_load(WebView *webview, const gchar *url, const gchar *content,
                  gboolean wholePageEditable) {
    WebKitWebView *widget = WEBKIT_WEB_VIEW(webview->widget);
    
    // Clean up from last time
    if (webview->link_blocker) {
        g_signal_handler_disconnect(widget, webview->link_blocker);
        webview->link_blocker = 0; // or is zero a valid handler?
    }
    
    GObject *settings = G_OBJECT(webkit_web_view_get_settings(WEBKIT_WEB_VIEW(widget)));
    g_object_set(settings, "enable-scripts", FALSE,  NULL);
    
    webview->common.wholePageEditable = wholePageEditable;
    
    // Load file
    webkit_web_view_load_string(widget, content, "text/html", NULL, url);
    
    // Disable links
    webview->link_blocker = g_signal_connect(widget,
            "navigation-policy-decision-requested",
            G_CALLBACK(block_navigation), NULL);
}


gchar *webview_getHTML(WebView *webview) {
    return webview_executeExpression(webview, "getHTML()");
}


void webview_setTitle(WebView *webview, const gchar *title) {
    gchar *escapedTitle = g_strescape(title, NULL);
    webview_executeFormattedScript(webview, "document.title = \"%s\";",
                                   escapedTitle);
    g_free(escapedTitle);
}


void webview_executeScript(WebView *webview, const gchar *script) {
    webkit_web_view_execute_script(WEBKIT_WEB_VIEW(webview->widget), script);
}


gchar *webview_executeExpression(WebView *webview, const gchar *expr) {
    WebKitWebView *widget = WEBKIT_WEB_VIEW(webview->widget);
    
    /*
        A "nicer" solution would be to catch "window-object-cleared" and add
        a special property/function there, but that's much more work for little
        practical benefit.
    */
    
    // Use the title to store the value of the expression
    webview_executeFormattedScript(webview, "var oldTitle = document.title;"
                                            "document.title = encodeURIComponent(%s);", expr);
    
    gchar *result = g_uri_unescape_string(webkit_web_view_get_title(widget), NULL);
    
    // Restore old title
    webview_executeScript(webview, "document.title = oldTitle; delete oldTitle;");
    return result;
}



