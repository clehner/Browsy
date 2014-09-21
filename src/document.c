#include <stdlib.h>
#include <string.h>
#include <Memory.h>
#include "document.h"
#include "window.h"
#include "tokenizer.h"

Node *NewNode() {
	return (Node *)calloc(1, sizeof(Node));
}

void DisposeNode(Node *node) {
	free(node);
}

void DisposeNodes(Node *node) {
	Node *curr;
	for (curr = node->firstChild; curr; curr = curr->nextSibling) {
		DisposeNodes(curr);
	}
	DisposeNode(node);
}

DOMDocument *NewDOMDocument() {
	DOMDocument *doc = (DOMDocument *)malloc(sizeof(DOMDocument));
	doc->data = NULL;
	doc->rootNode = NewNode();
	doc->tokenizer = NewTokenizer();
	return doc;
}

void DisposeDOMDocument(DOMDocument *doc) {
	DisposeNodes(doc->rootNode);
	DisposeTokenizer(doc->tokenizer);
	free(doc);
}

void DOMDocumentParseAppend(DOMDocument *doc, Ptr data, long bytes) {
	//tokenize(doc->tokenizer);

}
