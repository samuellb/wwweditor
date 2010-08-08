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

typedef struct {
    gboolean isTemplate;
    gchar *templateURI;
} FileInfo;

// Controller functions
void controller_setProjectPath(const gchar *path);

void controller_newDocument(const gchar *uri, const gchar *templateURI);
void controller_loadDocument(const gchar *uri);
void controller_closeDocument();

gboolean controller_canExit();
FileInfo *controller_getFileInfo(const gchar *uri);
void controller_freeFileInfo(FileInfo *status);


// These functions must be provided by the view
void view_showDirectory(const gchar *path);
void view_showDocument(const gchar *fileURL, const gchar *html);


#endif

