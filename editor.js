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

Array.prototype.contains = function(elem) { return this.indexOf(elem) != -1; };


// Remove event handlers
// TODO

// Make marked sections editable
var editableElements = [];

function makeNodeEditable(node, marker) {
    node.contentEditable = true;
    node.setAttribute("wwweditor:marker", marker);
    editableElements.push(node);
}

function isMarkerComment(node) {
    return (node.nodeType == Node.COMMENT_NODE &&
            node.nodeValue.search(/^\s*@\s*/) == 0);
}

function makeMarkedEditable(obj) {
    var inBeginning = true;
    for (var node = obj.firstChild; node != null; node = node.nextSibling) {
        // Skip empty text nodes
        if (node.nodeType == Node.TEXT_NODE && node.nodeValue.search(/^\s*$/) == 0)
            continue;
        
        // Look for special comments in the beginning
        if (inBeginning && isMarkerComment(node)) {
            var marker = node.nodeValue;
            makeNodeEditable(node.parentNode, marker);
        }
        inBeginning = false;
        
        // Process recursively
        makeMarkedEditable(node);
    }
}


function checkMarkers() {
    for (var i = 0; i < editableElements.length; i++) {
        var obj = editableElements[i];
        
        var hasMarker = false;
        for (var node = obj.firstChild; node != null; node = node.nextSibling) {
            // Skip empty text nodes
            if (node.nodeType == Node.TEXT_NODE && node.nodeValue.search(/^\s*$/) == 0)
                continue;
            
            // Look for special comments in the beginning
            if (isMarkerComment(node)) {
                hasMarker = true;
            }
            
            break; // Reached a tag or a comment
        }
        
        if (!hasMarker) {
            // Marker is gone!
            var marker = document.createComment(obj.getAttribute("wwweditor:marker"));
            obj.insertBefore(marker, obj.firstChild);
        }
    }
}


if (editor.wholePageEditable) {
    makeNodeEditable(document.documentElement);
} else {
    makeMarkedEditable(document.documentElement);
}


function nodeIsEditable(node) {
    while (node != null && !editableElements.contains(node)) {
        node = node.parentNode;
    }
    return node != null;
}


// Function to get HTML properly
function getHTML() {
    var root = document.documentElement;
    if (editor.wholePageEditable) root.removeAttribute("contenteditable");
    
    // Put back the marker comments if they are missing
    checkMarkers();
    
    // FIXME non-standard class
    var html = new XMLSerializer().serializeToString(document);
    
    if (editor.wholePageEditable) root.contentEditable = true;
    return html;
}


// Cursor move detection
var lastNode = null;
var i = 0;
function cursorMoved(e) {
    var curNode = window.getSelection().anchorNode;
    
    // Only show options for editable nodes
    if (!nodeIsEditable(curNode)) curNode = null;
    
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


// Sets the tag name of the current block-level element
function setElementType(elementName) {
    var blockElems = ["blockquote", "div", "h1", "h2", "h3", "h4", "h5", "h6", "p"];
    var specialElems = ["body", "form", "iframe", "td"];
    var stopElems = blockElems.concat(specialElems);
    
    var curNode = window.getSelection().anchorNode;
    if (curNode == null || !nodeIsEditable(curNode)) return;
    
    // Find the containing block-level element
    while (curNode.parentNode != null && (curNode.nodeType != Node.ELEMENT_NODE ||
                                          !stopElems.contains(curNode.nodeName.toLowerCase()))) {
        curNode = curNode.parentNode;
    }
    
    // Don't replace elements with special meaning or the root element
    var wrap = !curNode.parentNode ||
               specialElems.contains(curNode.nodeName.toLowerCase()) ||
               editableElements.contains(curNode);
    
    // Create an element of the new type
    var newElem = document.createElement(elementName);
    
    if (!wrap) {
        // Copy attributes
        var oldAttrs = curNode.attributes;
        for (var i = 0; i < oldAttrs.length; i++) {
            newElem.setAttribute(oldAttrs[i].nodeName, oldAttrs[i].nodeValue);
        }
    }
    
    // Move child nodes
    while (curNode.firstChild != null) {
        newElem.appendChild(curNode.firstChild);
    }
    
    if (!wrap) {
        // Delete the old element
        curNode.parentNode.replaceChild(newElem, curNode);
    } else {
        // Add the new node to this node
        curNode.appendChild(newElem);
    }
}


// Automatic cleanup of the HTML
var handleModifications = true;
function subtreeModified(event) {
    if (!handleModifications) return;
    handleModifications = false;
    
    for (var node = window.getSelection().anchorNode;
         node != null; node = node.parentNode) {
        if (node.nodeType == Node.ELEMENT_NODE) cleanup(node);
        var sibling = node.nextSibling;
        if (sibling != null && sibling.nodeType == Node.ELEMENT_NODE) cleanup(sibling);
    }
    
    handleModifications = true;
}

function handleBackspace(event) {
    // Workaround for WebKit
    if (event.keyCode == 8 || event.charCode == 8) {
        subtreeModified(null);
    }
}

function cleanup(elem) {
    var parent = elem.parentNode;
    if (elem.nodeName == "SPAN" && elem.className == "Apple-style-span" && parent != null) {
        // Remove <span> element and replace it with it's contents
        while (elem.firstChild != null) {
            parent.insertBefore(elem.firstChild, elem);
        }
        parent.removeChild(elem);
    }
}

document.documentElement.addEventListener("DOMSubtreeModified", subtreeModified, true);
document.documentElement.addEventListener("keypress", handleBackspace, true);
document.documentElement.addEventListener("keyup", handleBackspace, true);

if (editableElements.length > 0) {
    editableElements[0].focus();
}


