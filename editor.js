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

// Misc. functions
function emptyIfNull(s) { return (s != null ? s : ""); }

// Remove event handlers
// TODO

// Make marked sections editable
// TODO should work differently for template documents
function makeMarkedEditable(obj) {
    var inBeginning = true;
    for (var node = obj.firstChild; node != null; node = node.nextSibling) {
        // Skip empty text nodes
        if (node.nodeType == Node.TEXT_NODE && node.nodeValue.search(/^\s*$/) == 0)
            continue;
        
        // Look for special comments in the beginning
        if (node.nodeType == Node.COMMENT_NODE && inBeginning) {
            if (node.nodeValue.search(/^\s*@\s*/) == 0) {
                node.parentNode.contentEditable = true;
            }
        }
        inBeginning = false;
        
        // Process recursively
        makeMarkedEditable(node);
    }
}

makeMarkedEditable(document.documentElement);


// Cursor move detection
var lastNode = null;
var i = 0;
function cursorMoved(e) {
    var curNode = window.getSelection().anchorNode;
    
    // Only show options for editable nodes
    var editNode = curNode;
    while (editNode != null && (!editNode.contentEditable || editNode.contentEditable == "false")) {
        editNode = editNode.parentNode;
    }
    if (editNode == null) curNode = null;
    
    // Find the containing element
    while (curNode != null && curNode.nodeType != Node.ELEMENT_NODE) {
        curNode = curNode.parentNode;
    }
    
    // Ignore repeated events
    if (curNode == lastNode) return;
    lastNode = curNode;
    
    if (curNode == null) {
        window.status = "";
        return;
    }
    
    // Build info message
    var msg =
        curNode.tagName.toLowerCase()+"\n"+
        emptyIfNull(curNode.getAttribute("class"))+"\n"+
        emptyIfNull(curNode.getAttribute("href"))+"\n"+
        emptyIfNull(curNode.getAttribute("title"));
    
    window.status = msg;
}

// Handle selection DOM events
document.documentElement.addEventListener("focus", cursorMoved, true);
document.documentElement.addEventListener("mousedown", cursorMoved, true);
document.documentElement.addEventListener("mouseup", cursorMoved, true);
document.documentElement.addEventListener("keypress", cursorMoved, true);
document.documentElement.addEventListener("keydown", cursorMoved, true);
document.documentElement.addEventListener("keyup", cursorMoved, true);

// Handle selection with non-standard events for faster response
document.documentElement.addEventListener("selectstart", cursorMoved, true);


