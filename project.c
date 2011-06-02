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
#include <glib/gprintf.h>
#include <glib/gstdio.h>
#include "template.h"

#include "project.h"

struct Project_ {
    // Path to root directory
    gchar *path;
    
    // File states in GIT
    GData *fileStates;
};


Project *project_init(const gchar *path) {
    Project *project = g_malloc(sizeof(Project));
    project->path = (g_str_has_suffix(path, "/") ?
        g_strdup(path) : g_strconcat(path, "/", NULL));
    project->fileStates = NULL;
    
    project_refresh(project);
    return project;
}


void project_free(Project *project) {
    g_free(project->path);
    g_free(project);
}


static FileState gitStateToFileState(gchar x, gchar y) {
    switch (y) {
        case '?': return FileState_Unknown;
        case 'A': return FileState_Added;
        case 'C': return FileState_Copied;
        case 'D': return FileState_Deleted;
        case 'M': return FileState_Modified;
        case 'R': return FileState_Renamed;
        default: return FileState_Unmodified;
    }
}

void project_refresh(Project *project) {
    // Clear the file state list
    g_datalist_clear(&project->fileStates);
    g_datalist_init(&project->fileStates);
    
    // Read state from GIT
    
    // Spawn process: "git status --porcelain -z"
    static gchar *argv[] = { "git", "status", "--porcelain", "-z", NULL };
    GPid pid;
    gint output_fd;
    GError *error = NULL;
    
    if (!g_spawn_async_with_pipes(project->path, argv, NULL,
                       G_SPAWN_SEARCH_PATH | G_SPAWN_STDERR_TO_DEV_NULL,
                       NULL, NULL, &pid,
                       NULL, &output_fd, NULL, // pipes
                       &error)) {
        // TODO better error message
        g_fprintf(stderr, "failed to spawn process!\n");
        g_error_free(error);
        return;
    }
    
    // Create an IO channel so we can read from the output fd
    GIOChannel *chan = g_io_channel_unix_new(output_fd);
    if (!chan) {
        // TODO better error message
        g_fprintf(stderr, "failed to create IO channel!\n");
        g_spawn_close_pid(pid);
        return;
    }
    
    // Read the output
    error = NULL;
    gchar *data;
    gsize length;
    if (!g_io_channel_read_to_end(chan, &data, &length, &error)) {
        // TODO better error message
        g_fprintf(stderr, "failed to read from pipe!\n");
    }
    
    // Parse
    gchar *end = data + length;
    gchar *p = data;
    while (p && p != end && *p) {
        gsize entrylen = strlen(p);
        if (entrylen < 4) continue;
        
        // Parse status
        gchar x = p[0];
        gchar y = p[1];
        FileState state = gitStateToFileState(x, y);
        if (p[2] != ' ') continue;
        
        // Parse filename
        if (p[entrylen-1] == '/') p[entrylen-1] = '\0'; // remove trailing /
        p += 3;
        g_fprintf(stderr, "add filename >%s< = %d\n", p, state);
        g_datalist_set_data(&project->fileStates, p, (gpointer)state);
        
        p += entrylen - 3 + 1;
    }
    
    g_free(data);
    g_io_channel_unref(chan);
    
    // Clean up
    g_spawn_close_pid(pid);
}


const gchar *project_getPath(const Project *project) {
    return project->path;
}


gboolean project_isPage(Project *project, const gchar *uri) {
    return g_str_has_suffix(uri, ".html");
}


static gchar *readFile(const Project *project, const gchar *uri) {
    gchar *filename = g_strconcat(project->path, uri, NULL);
    gchar *contents = NULL;
    
    g_file_get_contents(filename, &contents, NULL, NULL);
    g_free(filename);
    return contents;
}


static gboolean saveFile(const Project *project, const gchar *uri,
                       const gchar *contents) {
    gchar *filename = g_strconcat(project->path, uri, NULL);
    gboolean ok = g_file_set_contents(filename, contents, -1, NULL);
    g_free(filename);
    return ok;
}


static const gchar *templateURI = "/template.html";
gboolean project_isTemplate(const Project *project, const gchar *uri) {
    return !strcmp(uri, templateURI);
}


gchar *project_getTemplateURI(const Project *project, const gchar *uri) {
    return g_strdup(!strcmp(uri, templateURI) ? NULL : templateURI);
}


FileState project_getFileState(Project *project, const gchar *uri) {
    // Skip leading / because it's not present in local paths,
    // for example from "git status"
    if (uri[0] == '/') uri++;
    
    g_printf("looking up >%s<\n", uri);
    return (FileState)g_datalist_get_data(&project->fileStates, uri);
}


gchar *project_getFileURL(const Project *project, const gchar *uri) {
    return g_strconcat("file://",
        (g_path_is_absolute(uri) ? "" : "./"),
        uri, NULL);
}


static gchar *mergeWithTemplate(const Project *project, const gchar *uri,
                                const gchar *contents) {
    if (!contents) return NULL;
    
    // Load template
    gchar *templateURI = project_getTemplateURI(project, uri);
    if (!templateURI) return g_strdup(contents);
    
    gchar *templateContents = project_loadPage(project, templateURI);
    g_free(templateURI);
    if (!templateContents) return g_strdup(contents);
    
    // Merge
    Template *tem = template_parseFromString(templateContents);
    gchar *page = template_updatePage(tem, contents);
    template_free(tem);
    
    g_free(templateContents);
    return page;
}


gchar *project_loadPage(const Project *project, const gchar *uri) {
    // Load contents
    gchar *contents = readFile(project, uri);
    
    // Merge with template
    gchar *page = mergeWithTemplate(project, uri, contents);
    g_free(contents);
    return page;
}


gboolean project_savePage(Project *project, const gchar *uri, const gchar *html) {
    // Restore and update the template parts
    gchar *page = mergeWithTemplate(project, uri, html);
    
    // Save file
    return saveFile(project, uri, page);
}


gboolean project_addPage(Project *project, const gchar *uri, const gchar *templateURI) {
    // TODO: set template URI
    return project_savePage(project, uri, "");
}

gboolean project_deletePage(Project *project, const gchar *uri) {
    gchar *filename = g_strconcat(project->path, uri, NULL);
    gboolean ok = (g_remove(filename) == 0);
    g_free(filename);
    return ok;
}

