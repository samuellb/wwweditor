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
#include "template.h"

#include "project.h"

struct Project_ {
    gchar *path;
};


Project *project_init(const gchar *path) {
    Project *project = g_malloc(sizeof(Project));
    project->path = (g_str_has_suffix(path, "/") ?
        g_strdup(path) : g_strconcat(path, "/", NULL));
    return project;
}


void project_free(Project *project) {
    g_free(project->path);
    g_free(project);
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


gchar *project_getTemplateURI(const Project *project, const gchar *uri) {
    static const gchar *templateURI = "/template.html";
    return g_strdup(!strcmp(uri, templateURI) ? NULL : templateURI);
}


gchar *project_getFileURL(const Project *project, const gchar *uri) {
    return g_strconcat("file://",
        (g_path_is_absolute(uri) ? "" : "./"),
        uri, NULL);
}


gchar *project_loadPage(const Project *project, const gchar *uri) {
    // Load contents
    gchar *contents = readFile(project, uri);
    if (!contents) return NULL;
    
    // Load template
    gchar *templateURI = project_getTemplateURI(project, uri);
    if (!templateURI) return contents;
    
    gchar *templateContents = project_loadPage(project, templateURI);
    g_free(templateURI);
    if (!templateContents) return contents;
    
    // Merge with template
    Template *tem = template_parseFromString(templateContents);
    gchar *page = template_updatePage(tem, contents);
    template_free(tem);
    
    g_free(templateContents);
    g_free(contents);
    return page;
}


gboolean project_savePage(Project *project, const gchar *uri, const gchar *html) {
    // TODO
    return FALSE;
}


