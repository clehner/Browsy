#ifndef _DOCUMENT_H
#define _DOCUMENT_H

#include "tokenizer.h"

typedef struct Node {
	struct Node *parentNode;
	struct Node *firstChild;
	struct Node *nextSibling;
} Node;

typedef struct {
	void *data;
	Node *rootNode;
	Tokenizer *tokenizer;
} DOMDocument;

Node *NewNode();
void DisposeNode(Node *node);
void DisposeNodes(Node *node);

DOMDocument *NewDOMDocument();
void DisposeDOMDocument(DOMDocument *doc);
void DOMDocumentParseAppend(DOMDocument *doc, Ptr data, long bytes);

#endif
