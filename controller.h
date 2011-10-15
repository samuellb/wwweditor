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

#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <glib.h>
#include "filestate.h"

typedef struct {
    gboolean isTemplate;
    gchar *templateURI;
    FileState state;
} FileInfo;

// Controller functions
gboolean controller_setProjectPath(const gchar *path);

void controller_newDocument(const gchar *uri, const gchar *templateURI);
void controller_loadDocument(const gchar *uri);
void controller_saveDocument();
void controller_closeDocument();

void controller_quit();
FileInfo *controller_getFileInfo(const gchar *uri);
void controller_freeFileInfo(FileInfo *status);

void controller_commitChanges();
void controller_discardChanges();

// These functions must be provided by the view
void view_showDirectory(const gchar *path);
void view_updateFileState(const gchar *path, const gchar *uri);
void view_setUncommited(gboolean uncommitted);

void view_showDocument(const gchar *fileURL,
                       const gchar *uri, const gchar *html,
                       gboolean wholePageEditable);
gchar *view_getDocumentHTML();
gchar *view_getDocumentFilename();

gboolean view_askCommit(gchar **message);
gboolean view_askDiscard();

void view_quit();


#endif

