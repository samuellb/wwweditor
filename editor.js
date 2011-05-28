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
    if (marker != null) {
        node.setAttribute("wwweditor:marker", marker);
    }
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
            var markerName = obj.getAttribute("wwweditor:marker");
            if (markerName != null) {
                var marker = document.createComment(markerName);
                obj.insertBefore(marker, obj.firstChild);
            }
        }
    }
}


if (editor.wholePageEditable) {
    makeNodeEditable(document.documentElement, null);
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


var blockElems = ["blockquote", "div", "h1", "h2", "h3", "h4", "h5", "h6", "p"];
var specialElems = ["body", "form", "iframe", "td"];
var stopElems = blockElems.concat(specialElems);

function findContainingBlock(node) {
    if (node == null || !nodeIsEditable(node)) return null;
    
    // Find the containing block-level element
    while (node.parentNode != null && !editableElements.contains(node) &&
           (node.nodeType != Node.ELEMENT_NODE ||
            !stopElems.contains(node.nodeName.toLowerCase()))) {
        node = node.parentNode;
    }
    
    return node;
}

function nextInTree(node) {
    if (node.firstChild != null) {
        // Go deeper down in the tree
        return node.firstChild;
    } else if (node.nextSibling != null) {
        // Go to next node
        return node.nextSibling;
    } else if (node.parentNode != null) {
        // Go up to the node after the parent
        return node.parentNode.nextSibling;
    } else {
        // Error!
        return null;
    }
}

// Returns the selected blocks level elements, or the block where the
// cursor is if there's no selection.
function getSelectedBlocks() {
    var blocks = [];
    var selection = window.getSelection();
    for (var i = 0; i < selection.rangeCount; i++) {
        var range = selection.getRangeAt(i);
        
        // Starting point in the tree
        var node = range.startContainer;
        // we ignore the start/endOffsets because we're not interested
        // in specific text segments, but only the paragraph as a whole.
        
        for (;;) {
            // Text nodes make up the selection
            // TODO it can contain some other things like images and objects
            if (node.nodeType == Node.TEXT_NODE && !node.nodeValue.match(/^\s*$/)) {
                var block = findContainingBlock(node);
                if (!blocks.contains(block)) {
                    blocks.push(block);
                }
            }
            
            // Check for end of selection
            if (node == range.endContainer) break;
            
            node = nextInTree(node);
        }
    }
    
    return blocks;
}


function getNextNonSpace(node) {
    do {
        node = node.nextSibling;
    } while (node != null &&
             node.nodeType == Node.TEXT_NODE &&
             node.nodeValue.match(/^\s*$/));
    
    return node;
}

function selectElements(elems) {
    // Clear selection
    var selection = document.getSelection();
    selection.removeAllRanges();
    
    // Place cursor in element if only one element is to be selected
    if (elems.length == 1) {
        var range = document.createRange();
        range.setStart(elems[0], 0);
        selection.addRange(range);
        return;
    }
    
    // More than one element is to be selected
    for (var i = 0; i < elems.length; i++) {
        var range = document.createRange();
        
        // Look ahead if this is a contiguous selection
        var j = i+1;
        for (; j < elems.length; j++) {
            var next = getNextNonSpace(elems[j-1]);
            if (getNextNonSpace(elems[j-1]) != elems[j]) break;
        }
        j--;
        
        if (j != i) {
            // Contiguous selection
            range.setStart(elems[i], 0);
            range.setEndAfter(elems[j]);
            i = j;
        } else {
            // Single block
            range.selectNode(elems[i]);
        }
        selection.addRange(range);
    }
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
    
    // Find out element type of the block
    var block = findContainingBlock(curNode);
    
    // Build info message
    var msg =
        block.tagName.toLowerCase()+"\n"+
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
    
    var blocks = getSelectedBlocks();
    var newBlocks = [];
    
    for (var i = 0; i < blocks.length; i++) {
        var block = blocks[i];
        
        // Don't replace elements with special meaning or the root element
        var wrap = !block.parentNode ||
                   specialElems.contains(block.nodeName.toLowerCase()) ||
                   editableElements.contains(block);
        
        // Create an element of the new type
        var newElem = document.createElement(elementName);
        
        if (!wrap) {
            // Copy attributes
            var oldAttrs = block.attributes;
            for (var j = 0; j < oldAttrs.length; j++) {
                newElem.setAttribute(oldAttrs[j].nodeName, oldAttrs[j].nodeValue);
            }
        }
        
        // Move child nodes
        while (block.firstChild != null) {
            newElem.appendChild(block.firstChild);
        }
        
        if (!wrap) {
            // Delete the old element
            block.parentNode.replaceChild(newElem, block);
        } else {
            // Add the new node to this node
            block.appendChild(newElem);
            newElem = block;
        }
        
        // The new block should be selected
        newBlocks.push(newElem);
    }
    
    // Select the new blocks
    selectElements(newBlocks);
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


