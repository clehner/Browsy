
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include <Memory.h>
#include <MacWindows.h>
#include <TextEdit.h>
#include <Resources.h>
#include "Browsy.h"
#include "uri.h"
#include "utils.h"
#include "window.h"

struct {
	char *name;
	short id;
} pageResources[] = {
	{"about:Browsy", 128},
	{"about:blank", 129}
};

char *GuessContentType(char *path) {
	char *extension = strrchr(path, '.');
	if (!extension)
		return "text/plain";
	if (!strcmp(extension, "html") || !strcmp(extension, "htm"))
		return "text/html";
	return "text/plain";
}

short getPageResourceId(char *uri) {
	if (!uri) return 0;
	for (int i = 0; i < SIZE(pageResources); i++) {
		if (!strcmp(uri, pageResources[i].name)) {
			return pageResources[i].id;
		}
	}
	return 0;
}

URIResponse *NewResponse() {
	return (URIResponse *)calloc(1, sizeof(URIResponse));
	/*URIResponse *resp;
	resp = (URIResponse *)malloc(sizeof(URIResponse));
	memset(resp, 0, sizeof(URIResponse);
	resp->headers = NULL;
	resp->offset = 0;
	resp->length = 0;
	resp->contentHandle = NULL;
	resp->contentType = NULL;
	return resp;*/
}

void DisposeResponse(URIResponse *resp) {
	if (resp->contentHandle) DisposeHandle(resp->contentHandle);
	if (resp->contentType) free(resp->contentType);
	DisposeHeaders(resp->headers);
	free(resp);
}

void DisposeHeaders(HTTPHeader *header) {
	if (!header) return;
	free(header->name);
	free(header->value);
	DisposeHeaders(header->next);
}

void DisposeRequest(URIRequest *req) {
	DisposeHeaders(req->headers);
	if (req->data) DisposeHandle(req->data);
	if (req->uri) free(req->uri);
	if (req->response) DisposeResponse(req->response);
	free(req);
};

void RequestURI(
	char *uri,
	void (*callback)(struct URIRequest *req),
	void *refCon)
{
	URIRequest *req;
	URIResponse *resp;
	char scheme[8]; // uri scheme
	short i;
	short len = strlen(uri)+1;
	char *path; // part of the uri after the scheme:

	req = (URIRequest *)malloc(sizeof(URIRequest));
	if (!req) {
		ErrorAlert("Unable to create URI request.");
		return;
	}

	req->refCon = refCon;
	req->callback = callback;
	req->uri = (char *)malloc(sizeof(char)*len);
	strncpy(req->uri, uri, len);

	// get scheme/protocol
	// and make it lowercase
	for (i=0; i<len && i<8 && uri[i] != ':'; i++) {
		scheme[i] = uri[i] = tolower(uri[i]);
	}
	scheme[i++] = 0;
	path = uri+i;

	if (strcmp(scheme, "file")==0) {
		char *filePath = url_decode(path);
		if (!filePath) {
			ErrorAlert("Unable to decode URI");
			return;
		}
		char *pathStart = filePath;
		while (pathStart[0] == '/') pathStart++;
		StringPtr fName = GetFilePathFileName(pathStart);
		int vRefNum = GetFilePathVolRef(pathStart);
		OSErr err;
		char errbuf[128];
		short refNum = 0;
		long bytes;
		Handle fileContents = NULL;

		/*
		alertf("[%d] %s vol: %d", fName[0], fName+1, vRefNum);
		snprintf(errbuf, sizeof errbuf, "[%d] %s vol: %d", fName[0], fName+1, vRefNum);
		ErrorAlert(errbuf);
		*/

		if ((err = FSOpen(fName, vRefNum, &refNum)) != noErr) {
			snprintf(errbuf, sizeof errbuf, "Unable to read file. err: %d", err);
			ErrorAlert(errbuf);
			//ErrorAlert("Unable to read file");
		} else if (GetEOF(refNum, &bytes) != noErr) {
			ErrorAlert("Unable to read file2"); //fName
		} else if ((fileContents=NewHandle(bytes))==NULL) {
			ErrorAlert("Not enough memory"); //fName
		} else if (FSRead(refNum, &bytes, *fileContents) != noErr) {
			ErrorAlert("Unable to read file3"); //fName
		} else {
			resp = NewResponse();
			resp->length = bytes;
			resp->contentHandle = fileContents;
			resp->contentType = GuessContentType(filePath);
			req->state = stateComplete;
			req->response = resp;
			callback(req);
			//TESetText(*fileContents, bytes, pWin->contentTE);
			//HLock(fileContents);
			//(*pWin->contentTE)->hText = fileContents;
			//TECalText(pWin->contentTE);
			//InvalRect(&(*pWin->contentTE)->viewRect);
			//HUnlock(fileContents);
		}
		if (refNum) FSClose(refNum);
		//if (fileContents) DisposeHandle(fileContents);
		free(filePath);

	} else if (strcmp(scheme, "http")==0) {
		Handle errorText = NewHandle(25);
		strcpy(*errorText, "http not yet implemented!");
		//char *path;
		//PageWindowSetStatus(pWin, "http not yet implemented!");
		resp = NewResponse();
		resp->contentHandle = errorText;
		resp->length = 25;
		resp->contentType = "text/plain";
		req->state = stateComplete;
		req->response = resp;
		callback(req);

	} else if (strcmp(scheme, "about")==0) {
		Handle text = NULL;
		short rsrcId = getPageResourceId(uri);
		if (rsrcId) {
			text = GetResource('TEXT', rsrcId);
		}
		resp = NewResponse();
		if (text == NULL) {
			resp->contentHandle = NULL;
			resp->length = 0;
		} else {
			resp->contentHandle = text;
			//ReleaseResource(text);
			resp->length = InlineGetHandleSize(text);
		}
		resp->contentType = "text/plain";
		req->state = stateComplete;
		req->response = resp;
		callback(req);

	} else {
		char errorText[] = "Unknown URI scheme.";
		//char *path;
		//PageWindowSetStatus(pWin, "http not yet implemented!");
		resp = NewResponse();
		resp->contentHandle = (Handle)&errorText;
		resp->length = sizeof(errorText);
		resp->contentType = "text/plain";
		req->state = stateComplete;
		req->response = resp;
		callback(req);
		//PageWindowSetStatus(pWin, "Unknown URI scheme.");
	}
}
