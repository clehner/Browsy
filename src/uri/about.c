#include <stdlib.h>
#include "stream.h"
#include "uri.h"
#include "uri/about.h"

void *AboutProviderInit(URI *uri, char *uriStr);
void AboutProviderClose(URI *uri, void *providerData);
void AboutProviderRequest(URI *uri, HTTPMethod *method, Stream *postData);

struct URIProvider *aboutURIProvider = &(URIProvider){
	.init = AboutProviderInit,
	.request = AboutProviderRequest,
	.close = AboutProviderClose
};

// create and return the provider data
void *AboutProviderInit(URI *uri, char *uriStr)
{
	return NULL;
}

void AboutProviderClose(URI *uri, void *providerData)
{
}

void AboutProviderRequest(URI *uri, HTTPMethod *method, Stream *postData)
{
	// ignore POST data
	(void)postData;

	if (method->type != httpGET) {
		// only GET is supported
		URIClosed(uri, uriBadMethodErr);
		return;
	}

	URIGotStatus(uri, 200);
	char msg[] = "about worked!\nyay\n";
	URIGotData(uri, msg, sizeof msg);
	URIClosed(uri, 0);
}

