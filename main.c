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
#include <gtk/gtk.h>
#include "project.h"
#include "webview.h"


// Active project
static Project *project;

// Main window
static GtkWidget *main_window;

// Web view
static WebView *webview;
static GtkWidget *webview_widget;

// Element properties
static GtkWidget *element_types_view;
static GtkWidget *styles_view;
static GtkWidget *link_expander;
static GtkWidget *link_href_entry;
static GtkWidget *link_title_entry;


void notifyFunction(WebView *view, const WebViewElementInfo *info) {
    gboolean isLink = !strcmp(info->tagName, "a");
    
    gtk_entry_set_text(GTK_ENTRY(link_href_entry), info->linkHref);
    gtk_entry_set_text(GTK_ENTRY(link_title_entry), info->title);
    gtk_widget_set_visible(link_expander, isLink);
}

int main(int argc, char **argv) {
    // Initialize GTK
    gtk_init(&argc, &argv);
    
    // Load the user interface
    GtkBuilder *builder = gtk_builder_new();
    GError *error = NULL;
    
    if (!gtk_builder_add_from_file(builder, "interface.xml", &error)) {
        g_error("Failed to open GtkBuilder XML: %s\n", error->message);
    }
    
    // Prepare WebKit
    webview = webview_new(notifyFunction);
    webview_widget = webview_getWidget(webview);
    gtk_widget_show(webview_widget);
    
    // Load project (TODO remove this code)
    project = project_init("test");
    gchar *html = project_loadPage(project, "page1.html");
    gchar *url = project_getFileURL(project, "page1.html");
    webview_load(webview, url, html);
    g_free(url);
    g_free(html);
    
    // Prepare main window
    main_window = GTK_WIDGET(gtk_builder_get_object(builder, "main_window"));
    GtkContainer *page_container = GTK_CONTAINER(gtk_builder_get_object(builder, "page_container"));
    gtk_container_add(page_container, webview_widget);
    
    element_types_view = GTK_WIDGET(gtk_builder_get_object(builder, "element_types_view"));
    styles_view = GTK_WIDGET(gtk_builder_get_object(builder, "styles_view"));
    link_expander = GTK_WIDGET(gtk_builder_get_object(builder, "link_expander"));
    link_href_entry = GTK_WIDGET(gtk_builder_get_object(builder, "link_href_entry"));
    link_title_entry = GTK_WIDGET(gtk_builder_get_object(builder, "link_title_entry"));
    
    g_signal_connect(main_window, "delete-event", gtk_main_quit, NULL);
    gtk_widget_show(main_window);
    
    gtk_main();
    return 0;
}


