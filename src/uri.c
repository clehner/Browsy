#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include <Memory.h>
#include <MacWindows.h>
#include <TextEdit.h>

#include "http_parser.h"
#include "stream.h"

#include "Browsy.h"
#include "uri.h"
#include "utils.h"
#include "window.h"

#include "uri/about.h"
#include "uri/file.h"
#include "uri/http.h"

/*
char *GuessContentType(char *path) {
	char *extension = strrchr(path, '.');
	if (!extension)
		return "text/plain";
	if (!strcmp(extension, "html") || !strcmp(extension, "htm"))
		return "text/html";
	return "text/plain";
}

void DisposeResponse(URIResponse *resp) {
	if (resp->contentHandle) DisposeHandle(resp->contentHandle);
	if (resp->contentType) free(resp->contentType);
	DisposeHeaders(resp->headers);
	free(resp);
}

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
		// skip initial slashes
		char *pathStart = filePath;
		while (pathStart[0] == '/') pathStart++;
		// get file name without leading directory components
		StringPtr fName = CtoPCopy(getFilePathFileName(pathStart));
		// get the volume
		int vRefNum = GetFilePathVolRef(pathStart);

		OSErr err;
		char errbuf[128];
		short refNum = 0;
		long bytes;
		Handle fileContents = NULL;

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
		DisposePtr(fName);

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
*/

#define HTTP_SCHEME_PROVIDERS \
X("about", aboutURIProvider) \
X("file", fileURIProvider) \
X("http", httpURIProvider)

// URI object: a remote resource that may be requested and may respond
struct URI {
	URIConsumer *consumer;
	void *consumerData;
	URIProvider *provider;
	void *providerData;
};

// create a new uri object
URI *NewURI(char *uriStr)
{
	URIProvider *provider;
	URI *uri;
	
	provider = URIGetProvider(uriStr);
	if (!provider) return NULL;

	uri = malloc(sizeof(URI));
	if (!uri) return NULL;

	URIProvide(uri, provider, uriStr);
	return uri;
}

URIProvider *URIGetProvider(char *uri)
{
	short i;
	struct URISchemeProvider *provider;

#define X(scheme, provider) \
	if (strncmp(uri, scheme, sizeof(scheme)-1) == 0 \
			&& uri[sizeof(scheme)-1] == ':') { \
		return provider; \
	}
HTTP_SCHEME_PROVIDERS

	return NULL;

	/*
	struct http_parser_url u;
	int result;
	if ((result = http_parser_parse_url(uriStr, strlen(uriStr), 0, &u))) {
	  alertf("Unable to parse URL %s: %d", uriStr, result);
	  free(uri);
	  return NULL;
	}
	*/
}

// set the uri consumer
void URIConsume(URI *uri, URIConsumer *consumer, void *consumerData)
{
	uri->consumer = consumer;
	uri->consumerData = consumerData;
}

// set the uri provider
void URIProvide(URI *uri, URIProvider *provider, char *uriStr)
{
	uri->provider = provider;
	uri->providerData = provider->init(uri, uriStr);
}

// request the URI, optionally sending along some data
void URIRequest(URI *uri, char *method, Stream *postData)
{
	uri->provider->request(uri, uri->providerData, &(HTTPMethod){httpOtherMethod, method},
			postData);
}

// GET the URI
void URIGet(URI *uri)
{
	uri->provider->request(uri, uri->providerData, &(HTTPMethod){httpGET}, NULL);
}

// POST to the URI
void URIPost(URI *uri, Stream *postData)
{
	uri->provider->request(uri, uri->providerData, &(HTTPMethod){httpPOST}, NULL);
}

// forcibly close the uri request and response
void URIClose(URI *uri)
{
	uri->provider->close(uri, uri->providerData);
}

// Provider methods:

void URIGotStatus(URI *uri, short status)
{
	uri->consumer->on_status(uri->consumerData, status);
}

void URIGotHeader(URI *uri, struct HTTPHeader *header)
{
	uri->consumer->on_header(uri->consumerData, header);
}

void URIGotData(URI *uri, char *data, short len)
{
	uri->consumer->on_data(uri->consumerData, data, len);
}

void URIClosed(URI *uri, short error)
{
	uri->consumer->on_close(uri->consumerData, error);
}

