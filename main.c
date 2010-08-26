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
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include "controller.h"
#include "webview.h"


static GtkBuilder *builder;
static GtkWidget *main_window;

// Actions
static GtkActionGroup *projectActions;
static GtkActionGroup *documentActions;

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
static GtkListStore *elementTypes;
static GtkTreeView *elementTypesView;

typedef enum {
    ElementColumn_DisplayName = 0,
    ElementColumn_TagName,
} ElementTypesColumnId;

static GtkWidget *styles_view;
static GtkWidget *link_expander;
static GtkWidget *link_href_entry;
static GtkWidget *link_title_entry;


static void notifyFunction(WebView *view, const WebViewElementInfo *info) {
    // Update element selection
    GtkTreeModel *elementsModel = GTK_TREE_MODEL(elementTypes);
    GtkTreeSelection *elementsSelection = gtk_tree_view_get_selection(elementTypesView);
    GtkTreeIter iter;
    gboolean valid = gtk_tree_model_get_iter_first(elementsModel, &iter);
    while (valid) {
        gchar *tagName;
        gtk_tree_model_get(elementsModel, &iter,
                           ElementColumn_TagName, &tagName, -1);
        gint diff = strcmp(info->tagName, tagName);
        g_free(tagName);
        
        if (!diff) {
            // Select
            gtk_tree_selection_select_iter(elementsSelection, &iter);
            break;
        }
        valid = gtk_tree_model_iter_next(elementsModel, &iter);
    }
    
    if (!valid) {
        // Clear selection
        gtk_tree_selection_unselect_all(elementsSelection);
    }
    
    // Update link view
    gboolean isLink = !strcmp(info->tagName, "a");
    gtk_entry_set_text(GTK_ENTRY(link_href_entry), info->linkHref);
    gtk_entry_set_text(GTK_ENTRY(link_title_entry), info->title);
    gtk_widget_set_visible(link_expander, isLink);
}


static void elementTypeSelected(GtkTreeView *tree_view, GtkTreePath *path,
                                GtkTreeViewColumn *column, gpointer user_data) {
    GtkTreeModel *model = GTK_TREE_MODEL(elementTypes);
    GtkTreeIter iter;
    gchar *tagName;
    
    gtk_tree_model_get_iter(model, &iter, path);
    gtk_tree_model_get(model, &iter, ElementColumn_TagName, &tagName, -1);
    
    fprintf(stderr, "selected %s!\n", tagName);
    gchar *script = g_strdup_printf("setElementType('%s');", tagName);
    webview_executeScript(webview, script);
    
    g_free(script);
    g_free(tagName);
}



static void fileSelected(GtkTreeView *tree_view, GtkTreePath *path,
                         GtkTreeViewColumn *column, gpointer user_data) {
    GtkTreeModel *model = GTK_TREE_MODEL(fileTree);
    GtkTreeIter iter;
    gchar *uri;
    
    gtk_tree_model_get_iter(model, &iter, path);
    gtk_tree_model_get(model, &iter, FileColumn_URI, &uri, -1);
    
    controller_loadDocument(uri);
    
    g_free(uri);
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
        g_error_free(error);
        return;
    }
    
    const gchar *entry;
    while ((entry = g_dir_read_name(dir)) != NULL) {
        if (entry[0] == '.') continue; // Skip hidden files
        
        GtkTreeIter iter;
        gtk_tree_store_append(fileTree, &iter, parent);
        
        gchar *filename = g_build_filename(path, entry, NULL);
        gchar *uri = g_build_path("/", baseUri, entry, NULL);
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
        g_free(filename);
    }
    g_dir_close(dir);
}

void view_showDirectory(const gchar *path) {
    gtk_tree_store_clear(fileTree);
    gtk_action_group_set_sensitive(projectActions, (path != NULL));
    if (!path) return;
    
    addDirectory(NULL, path, "/");
}


void view_showDocument(const gchar *fileURL, const gchar *html,
                       gboolean wholePageEditable) {
    if (html) webview_load(webview, fileURL, html, wholePageEditable);
    else webview_load(webview, "", "", FALSE);
    
    gtk_action_group_set_sensitive(documentActions, (html != NULL));
}


gchar *view_getDocumentHTML() {
    return webview_getHTML(webview);
}


static void actionSavePage(GtkAction *action, gpointer user_data) {
    controller_saveDocument();
}


static void setAction(const gchar *actionName,
                      void (*handler)(GtkAction *action, gpointer user_data)) {
    GtkAction *action = GTK_ACTION(gtk_builder_get_object(builder, actionName));
    g_signal_connect(action, "activate", G_CALLBACK(handler), NULL);
}


int main(int argc, char **argv) {
    GError *error = NULL;
    static gchar **paths = NULL;
    static const GOptionEntry entries[] = {
        { G_OPTION_REMAINING, 0, G_OPTION_FLAG_FILENAME,
          G_OPTION_ARG_FILENAME_ARRAY, &paths, NULL, "DIRECTORY" },
        { NULL, 0, 0, 0, NULL, NULL, NULL, }
    };
    
    // Parse options
    GOptionContext *context = g_option_context_new("");
    g_option_context_set_summary(context, "Edit web sites");
    g_option_context_add_main_entries(context, entries, NULL);
    g_option_context_add_group(context, gtk_get_option_group(TRUE));
    if (!g_option_context_parse(context, &argc, &argv, &error)) {
        g_printf("%s\n", error->message);
        exit(2);
    }
    
    if (paths && paths[0] && paths[1]) {
        g_printf("More than one project directory specified\n");
        exit(2);
    }
    
    // Initialize locale and GTK
    setlocale(LC_ALL, "");
    gtk_init(&argc, &argv);
    
    // Load the user interface
    builder = gtk_builder_new();
    if (!gtk_builder_add_from_file(builder, "interface.xml", &error)) {
        g_error("Failed to open GtkBuilder XML: %s\n", error->message);
    }
    
    // Prepare WebKit
    webview = webview_new(notifyFunction);
    webview_widget = webview_getWidget(webview);
    gtk_widget_show(webview_widget);
    
    // Prepare main window
    main_window = GTK_WIDGET(gtk_builder_get_object(builder, "main_window"));
    
    // Prepare actions
    projectActions = GTK_ACTION_GROUP(gtk_builder_get_object(builder, "project_actions"));
    documentActions = GTK_ACTION_GROUP(gtk_builder_get_object(builder, "document_actions"));
    
    // TODO set up handlers here
    setAction("action_save_page", actionSavePage);
    
    // Prepare file tree
    GtkTreeView *fileTreeView = GTK_TREE_VIEW(gtk_builder_get_object(builder, "file_tree_view"));
    fileTree = GTK_TREE_STORE(gtk_builder_get_object(builder, "file_tree"));
    
    GtkTreeViewColumn *fileColumn = gtk_tree_view_column_new();
    GtkCellRenderer *iconRenderer = GTK_CELL_RENDERER(gtk_cell_renderer_pixbuf_new());
    gtk_tree_view_column_pack_start(fileColumn, iconRenderer, FALSE);
    gtk_tree_view_column_add_attribute(fileColumn, iconRenderer,
        "stock-id", FileColumn_Icon);
    GtkCellRenderer *filenameRenderer = GTK_CELL_RENDERER(gtk_cell_renderer_text_new());
    gtk_tree_view_column_pack_start(fileColumn, filenameRenderer, TRUE);
    gtk_tree_view_column_add_attribute(fileColumn, filenameRenderer,
        "text", FileColumn_DisplayName);
    
    gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(fileTree),
        FileColumn_DisplayName, fileSortFunc, NULL, NULL);
    //gtk_tree_view_column_set_sort_column_id(column, FileColumn_DisplayName);
    gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(fileTree), FileColumn_DisplayName, GTK_SORT_ASCENDING);
    gtk_tree_view_append_column(fileTreeView, fileColumn);
    
    g_signal_connect(fileTreeView, "row-activated", G_CALLBACK(fileSelected), NULL);
    
    // Prepare document view
    GtkContainer *page_container = GTK_CONTAINER(gtk_builder_get_object(builder, "page_container"));
    gtk_container_add(page_container, webview_widget);
    
    // Prepare the right sidebar
    elementTypesView = GTK_TREE_VIEW(gtk_builder_get_object(builder, "element_types_view"));
    elementTypes = GTK_LIST_STORE(gtk_builder_get_object(builder, "element_types"));
    
    GtkTreeViewColumn *elementColumn = gtk_tree_view_column_new_with_attributes(
        "", GTK_CELL_RENDERER(gtk_cell_renderer_text_new()),
        "text", ElementColumn_DisplayName, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(elementTypesView), elementColumn);
    
    // TODO track selection or use radio button instead?
    g_signal_connect(elementTypesView, "row-activated", G_CALLBACK(elementTypeSelected), NULL);
    
    styles_view = GTK_WIDGET(gtk_builder_get_object(builder, "styles_view"));
    link_expander = GTK_WIDGET(gtk_builder_get_object(builder, "link_expander"));
    link_href_entry = GTK_WIDGET(gtk_builder_get_object(builder, "link_href_entry"));
    link_title_entry = GTK_WIDGET(gtk_builder_get_object(builder, "link_title_entry"));
    
    g_signal_connect(main_window, "delete-event", gtk_main_quit, NULL);
    
    // Load project on command line, if any
    controller_setProjectPath(paths ? paths[0] : NULL);
    
    // Show window
    gtk_widget_show(main_window);
    gtk_main();
    return 0;
}


