
wwweditor
=========

Description
-----------
wwweditor is a web site editor. Some features:

  * WYSIWYG editing.
  * Supports multiple pages in a "project"
  * Templates:
      - Different directories use a different template.
      - Updated automatically
  * Version control with GIT


Limitations
-----------

  * Not yet possible to create templates or directories.
  * No git push/pull.
  * No FTP or SSH support.

Also, you can't use wwweditor to create your web design. It's not meant
to do that.


How to use
----------
Find or create a layout for your site. This should consist of a single HTML
file and associated style sheets and graphical elements. Rename the HTML
file to template.html and the files in a new directory. Edit the template
to make it editable (see below).

Optionally, you may keep your web site under version control with GIT.
wwweditor can't create new repositories yet, so you'll have to do that
yourself and add all layout files.


How to make an existing HTML file an editable template
------------------------------------------------------
This is still not possible to do from the user interface. Open the HTML
code in a text editor and locate the tag which should be editable. Then
change it as follows:

    <div> from this </div>
    <div><!-- @contents --> to this </div>

You should only add the <!-- @contents --> marker on tags such as `<div>`
or `<td>`. If the text you want to make editable is inside a formatting tag
(like `<span>` and `<b>`), a paragraph (`<p>`), header tag (h1-h6) or some
other tag with special meaning, then you can wrap it in a `<div>` tag first.
Also, never put the marker on a `<table>`, `<tr>` or `<textarea>` tag!

You may also want to add a menu with link to your sub pages, so you don't
have to add it in each page.

If you want the change the layout, then just replace the template and open
and save all pages in your project. Warning! A lot of things can go wrong
during this process! Please make a backup (or make sure everthing is
committed in GIT, if you use that).

