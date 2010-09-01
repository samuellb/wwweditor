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

#include "project.h"

#include "controller.h"


// Active project
static Project *activeProject = NULL;
static gchar *activeDocument = NULL;
static gchar *activeTemplate = NULL;


static gboolean askSave() {
    // TODO show "Do you want to save?" dialog
    return TRUE;
}

static gboolean askSaveAndClose() {
    if (!askSave()) return FALSE;
    g_free(activeDocument);
    g_free(activeTemplate);
    activeDocument = NULL;
    activeTemplate = NULL;
    return TRUE;
}


void controller_setProjectPath(const gchar *path) {
    if (!askSaveAndClose()) return;
    view_showDocument(NULL, NULL, FALSE);
    
    activeProject = (path ? project_init(path) : NULL);
    view_showDirectory(path);
}


void controller_newDocument(const gchar *uri, const gchar *templateURI) {
    // TODO
}


void controller_loadDocument(const gchar *uri) {
    if (!askSaveAndClose()) return;
    
    activeDocument = g_strdup(uri);
    activeTemplate = project_getTemplateURI(activeProject, uri);
    
    gchar *html = project_loadPage(activeProject, uri);
    gchar *fileURL = project_getFileURL(activeProject, uri);
    gboolean wholePageEditable = (activeTemplate == NULL);
    view_showDocument(fileURL, html, wholePageEditable);
    g_free(fileURL);
    g_free(html);
}


void controller_saveDocument() {
    gchar *html = view_getDocumentHTML();
    project_savePage(activeProject, activeDocument, html);
    g_free(html);
}


void controller_closeDocument() {
    if (!askSaveAndClose()) return;
    view_showDocument(NULL, NULL, FALSE);
}



void controller_quit() {
    if (askSave()) {
        view_quit();
    }
}


FileInfo *controller_getFileInfo(const gchar *uri) {
    FileInfo *info = g_malloc(sizeof(FileInfo));
    info->isTemplate = project_isTemplate(activeProject, uri);
    info->templateURI = project_getTemplateURI(activeProject, uri);
    return info;
}


void controller_freeFileInfo(FileInfo *status) {
    g_free(status->templateURI);
    g_free(status);
}


