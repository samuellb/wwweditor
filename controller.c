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


gboolean controller_askSave() {
    // TODO show "Do you want to save?" dialog
    return TRUE;
}

static void freeDocument() {
    g_free(activeDocument);
    g_free(activeTemplate);
    activeDocument = NULL;
    activeTemplate = NULL;
}


gboolean controller_setProjectPath(const gchar *path) {
    if (!controller_askSave()) return FALSE;
    
    Project *project = NULL;
    if (path) {
        project = project_init(path);
        if (!project) {
            // TODO display an error message here
            return FALSE;
        }
    }
    
    // Free previous project
    freeDocument();
    activeProject = project;
    
    // Update view
    view_showDirectory(path);
    view_showDocument(NULL, NULL, FALSE);
    return (path == NULL || activeProject != NULL);
}


void controller_newDocument(const gchar *uri, const gchar *templateURI) {
    // TODO
}


void controller_loadDocument(const gchar *uri) {
    if (!controller_askSave()) return;
    
    // Try to load the page
    gchar *html = project_loadPage(activeProject, uri);
    if (!html) {
        // TODO show error message
        return;
    }
    
    // Free the previous document
    freeDocument();
    
    // Make the new page the current page
    activeDocument = g_strdup(uri);
    activeTemplate = project_getTemplateURI(activeProject, uri);
    
    // Update the view
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
    if (!controller_askSave()) return;
    
    // Close document
    freeDocument();
    
    // Update view
    view_showDocument(NULL, NULL, FALSE);
}



void controller_quit() {
    if (controller_askSave()) {
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


