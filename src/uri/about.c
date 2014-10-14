#include <stdlib.h>
#include <string.h>
#include <MacTypes.h>
#include <Memory.h>
#include <Resources.h>
#include "stream.h"
#include "utils.h"
#include "uri.h"
#include "uri/about.h"

void *AboutProviderInit(URI *uri, char *uriStr);
void AboutProviderClose(URI *uri, void *providerData);
void AboutProviderRequest(URI *uri, void *providerData, HTTPMethod *method,
		Stream *postData);

struct URIProvider *aboutURIProvider = &(URIProvider) {
	.init = AboutProviderInit,
	.request = AboutProviderRequest,
	.close = AboutProviderClose
};

struct {
	char *name;
	short id;
} pageResources[] = {
	{"about:Browsy", 128},
	{"about:blank", 129},
	{"about:stuff", 130},
};

short getPageResourceId(char *uri)
{
	if (!uri) return 0;
	for (int i = 0; i < SIZE(pageResources); i++) {
		if (!strcmp(uri, pageResources[i].name)) {
			return pageResources[i].id;
		}
	}
	return 0;
}

// create and return the provider data
void *AboutProviderInit(URI *uri, char *uriStr)
{
	size_t len = strlen(uriStr) + 1;
	char *data = malloc(len);
	if (!data) return NULL;
	strncpy(data, uriStr, len);
	return data;
}

void AboutProviderClose(URI *uri, void *providerData)
{
	free(providerData);
}

void AboutProviderRequest(URI *uri, void *providerData, HTTPMethod *method,
		Stream *postData)
{
	Handle text = NULL;
	short rsrcId;
	char *uriStr = (char *)providerData;

	// ignore POST data
	(void)postData;

	if (method->type != httpGET) {
		// only GET is supported
		URIClosed(uri, uriBadMethodErr);
		return;
	}

	rsrcId = getPageResourceId(uriStr);
	if (rsrcId) {
		text = GetResource('TEXT', rsrcId);
	}
	if (text == NULL) {
		URIGotStatus(uri, 404);
	} else {
		URIGotStatus(uri, 200);
		URIGotHeader(uri, &(HTTPHeader){httpContentType, "text/plain"});
		HLock(text);
		URIGotData(uri, *text, InlineGetHandleSize(text));
		HUnlock(text);
		ReleaseResource(text);
	}
	URIClosed(uri, 0);
	free(providerData);
}

