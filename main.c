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

#include <locale.h>
#include <string.h>
#include <gtk/gtk.h>
#include "controller.h"
#include "webview.h"


// Main window
static GtkWidget *main_window;

// File tree
static GtkTreeStore *fileTree;
#define ICON_DIR       GTK_STOCK_DIRECTORY
#define ICON_PAGE      GTK_STOCK_FILE
#define ICON_TEMPLATE  GTK_STOCK_COPY
#define ICON_OTHER     NULL

typedef enum {
    FileColumn_Icon = 0,
    FileColumn_DisplayName,
    FileColumn_URI,
    FileColumn_IsDirectory,
} FileTreeColumnId;

// Web view
static WebView *webview;
static GtkWidget *webview_widget;

// Element properties
static GtkWidget *element_types_view;
static GtkWidget *styles_view;
static GtkWidget *link_expander;
static GtkWidget *link_href_entry;
static GtkWidget *link_title_entry;


static void notifyFunction(WebView *view, const WebViewElementInfo *info) {
    gboolean isLink = !strcmp(info->tagName, "a");
    
    gtk_entry_set_text(GTK_ENTRY(link_href_entry), info->linkHref);
    gtk_entry_set_text(GTK_ENTRY(link_title_entry), info->title);
    gtk_widget_set_visible(link_expander, isLink);
}


static void fileSelected(GtkTreeView *tree_view, GtkTreePath *path,
                         GtkTreeViewColumn *column, gpointer user_data) {
    GtkTreeModel *model = GTK_TREE_MODEL(fileTree);
    GtkTreeIter iter;
    gchar *uri;
    
    gtk_tree_model_get_iter(model, &iter, path);
    gtk_tree_model_get(model, &iter, FileColumn_URI, &uri, -1);
    
    controller_loadDocument(uri);
}


static gint compareFilenames(const gchar *a, const gchar *b) {
    gchar *keyA = g_utf8_collate_key_for_filename(a, -1);
    gchar *keyB = g_utf8_collate_key_for_filename(b, -1);
    gint result = strcmp(keyA, keyB);
    g_free(keyB);
    g_free(keyA);
    return result;
}


static gint fileSortFunc(GtkTreeModel *model, GtkTreeIter *a,
                         GtkTreeIter *b, gpointer user_data) {
    // Directories come first
    gboolean dirA, dirB;
    gtk_tree_model_get(model, a, FileColumn_IsDirectory, &dirA, -1);
    gtk_tree_model_get(model, b, FileColumn_IsDirectory, &dirB, -1);
    if (dirA) { if (!dirB) return -1; }
    else       { if (dirB) return 1; }
    
    // Sort by filename
    gchar *uriA, *uriB;
    gtk_tree_model_get(model, a, FileColumn_URI, &uriA, -1);
    gtk_tree_model_get(model, b, FileColumn_URI, &uriB, -1);
    
    gint res = compareFilenames(uriA, uriB);
    
    g_free(uriA);
    g_free(uriB);
    return res;
}

static void addDirectory(GtkTreeIter *parent,
                         const gchar *path, const gchar *baseUri) {
    // TODO use change notification
    GError *error = NULL;
    GDir *dir = g_dir_open(path, 0, &error);
    if (!dir) {
        g_warning("%s: %s", path, error->message);
        return;
    }
    
    const gchar *entry;
    while ((entry = g_dir_read_name(dir)) != NULL) {
        if (entry[0] == '.') continue; // Skip hidden files
        
        GtkTreeIter iter;
        gtk_tree_store_append(fileTree, &iter, parent);
        
        gchar *filename = g_build_filename(path, entry, NULL);
        gchar *uri = g_build_filename(baseUri, entry, NULL);
        const gchar *icon;
        gboolean isDir = g_file_test(filename, G_FILE_TEST_IS_DIR);
        if (isDir) {
            // Add subdirectories
            addDirectory(&iter, filename, uri);
            icon = ICON_DIR;
        } else {
            FileInfo *info = controller_getFileInfo(uri);
            if (info->isTemplate) icon = ICON_TEMPLATE;
            else if (info->templateURI) icon = ICON_PAGE;
            else icon = ICON_OTHER;
            controller_freeFileInfo(info);
        }
        
        gtk_tree_store_set(fileTree, &iter,
                           FileColumn_Icon, icon,
                           FileColumn_DisplayName, entry,
                           FileColumn_URI, uri,
                           FileColumn_IsDirectory, isDir,
                           -1);
        
        g_free(uri);
    }
    g_dir_close(dir);
}

void view_showDirectory(const gchar *path) {
    gtk_tree_store_clear(fileTree);
    if (!path) return;
    
    addDirectory(NULL, path, "/");
}


void view_showDocument(const gchar *fileURL, const gchar *html) {
    if (html) webview_load(webview, fileURL, html);
    else webview_load(webview, "", "");
}


int main(int argc, char **argv) {
    // Initialize locale and GTK
    setlocale(LC_ALL, "");
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
    
    // Prepare main window
    main_window = GTK_WIDGET(gtk_builder_get_object(builder, "main_window"));
    
    GtkTreeView *fileTreeView = GTK_TREE_VIEW(gtk_builder_get_object(builder, "file_tree_view"));
    fileTree = GTK_TREE_STORE(gtk_builder_get_object(builder, "file_tree"));
    
    GtkTreeViewColumn *column = gtk_tree_view_column_new();
    GtkCellRenderer *iconRenderer = GTK_CELL_RENDERER(gtk_cell_renderer_pixbuf_new());
    gtk_tree_view_column_pack_start(column, iconRenderer, FALSE);
    gtk_tree_view_column_add_attribute(column, iconRenderer,
        "stock-id", FileColumn_Icon);
    GtkCellRenderer *filenameRenderer = GTK_CELL_RENDERER(gtk_cell_renderer_text_new());
    gtk_tree_view_column_pack_start(column, filenameRenderer, TRUE);
    gtk_tree_view_column_add_attribute(column, filenameRenderer,
        "text", FileColumn_DisplayName);
    
    gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(fileTree),
        FileColumn_DisplayName, fileSortFunc, NULL, NULL);
    //gtk_tree_view_column_set_sort_column_id(column, FileColumn_DisplayName);
    gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(fileTree), FileColumn_DisplayName, GTK_SORT_ASCENDING);
    gtk_tree_view_append_column(fileTreeView, column);
    
    g_signal_connect(fileTreeView, "row-activated", G_CALLBACK(fileSelected), NULL);
    
    GtkContainer *page_container = GTK_CONTAINER(gtk_builder_get_object(builder, "page_container"));
    gtk_container_add(page_container, webview_widget);
    
    element_types_view = GTK_WIDGET(gtk_builder_get_object(builder, "element_types_view"));
    styles_view = GTK_WIDGET(gtk_builder_get_object(builder, "styles_view"));
    link_expander = GTK_WIDGET(gtk_builder_get_object(builder, "link_expander"));
    link_href_entry = GTK_WIDGET(gtk_builder_get_object(builder, "link_href_entry"));
    link_title_entry = GTK_WIDGET(gtk_builder_get_object(builder, "link_title_entry"));
    
    g_signal_connect(main_window, "delete-event", gtk_main_quit, NULL);
    
    // Load project
    // TODO use path from command line
    controller_setProjectPath("test/");
    
    // Show window
    gtk_widget_show(main_window);
    gtk_main();
    return 0;
}


