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

#ifndef PROJECT_H
#define PROJECT_H

#include <glib.h>
#include "filestate.h"

typedef struct Project_ Project;

Project *project_init(const gchar *path);
void project_free(Project *project);
void project_refresh(Project *project);

const gchar *project_getPath(const Project *project);

gboolean project_isTemplate(const Project *project, const gchar *uri);
gchar *project_getTemplateURI(const Project *project, const gchar *uri);
FileState project_getFileState(Project *project, const gchar *uri);
gchar *project_getFileURL(const Project *project, const gchar *uri);

gchar *project_loadPage(const Project *project, const gchar *uri);
gboolean project_savePage(Project *project, const gchar *uri, const gchar *html);
gboolean project_addPage(Project *project, const gchar *uri, const gchar *templateURI);
gboolean project_addFile(Project *project, const gchar *uri);
gboolean project_deletePage(Project *project, const gchar *uri);

gboolean project_hasUncommitted(const Project *project);
gboolean project_commit(Project *project, const gchar *message);
gboolean project_discard(Project *project);

#endif

