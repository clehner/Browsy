#ifndef _URI_H
#define _URI_H

/*
typedef enum {
	stateResolving,
	stateConnecting,
	stateLoading,
	stateComplete
} URIRequestState;

typedef struct {
	short status;
	long length;
	long offset;
	char *contentType;
	HTTPHeader *headers;
	Handle contentHandle;
} URIResponse;

typedef struct URIRequest {
	char method;
	char *uri;
	HTTPHeader *headers;  // list of headers
	Handle data;          // POST data
	void (*callback)(struct URIRequest*); // called on data recieve and done
	void *refCon;         // User/app reference data
	URIRequestState state;
	URIResponse *response;
} URIRequest;
*/

/*
URIResponse *NewResponse();
void DisposeResponse(URIResponse *resp);
void DisposeHeaders(HTTPHeader *header);
void DisposeRequest(URIRequest *req);
void Request(char *uri, void (*callback)(URIRequest *req), void *refCon);
*/

#include <stdbool.h>

struct Stream;

typedef struct URI URI;
typedef struct URIConsumer URIConsumer;
typedef struct URIProvider URIProvider;

enum URIError {
	uriBadMethodErr = -1,
};

typedef struct HTTPHeader {
	enum {
		httpContentType,
		httpContentLength,
	} name;
	char *value;
} HTTPHeader;

typedef struct HTTPMethod {
	enum {
		httpGET,
		httpPOST,
		httpOtherMethod
	} type;
	char *value;
} HTTPMethod;

// callbacks for consuming a stream
struct URIConsumer {
	void (*on_status)(void *consumerData, short httpStatus);
	void (*on_header)(void *consumerData, struct HTTPHeader *header);
	void (*on_data)(void *consumerData, char *data, short len);
	void (*on_close)(void *consumerData, short err);
};

// functions for writing and receiving from a stream
struct URIProvider {
	void *(*init)(URI *uri, char *uriStr);
	void (*request)(URI *uri, void *providerData, struct HTTPMethod *method,
			struct Stream *postData);
	void (*close)(URI *uri, void *providerData);
	/*
	void (*write)(Stream *s, void *pData, char *data, unsigned short len);
	void (*poll)(Stream *s, void *providerData);
	*/
};

URIProvider *URIGetProvider(char *uri);

URI *NewURI(char *uri);
void URIConsume(URI *uri, URIConsumer *consumer, void *consumerData);
void URIClose(URI *uri);
void URIRequest(URI *uri, char *method, struct Stream *postData);
void URIGet(URI *uri);
void URIPost(URI *uri, struct Stream *postData);

// call by provider
void URIProvide(URI *uri, URIProvider *provider, char *uriStr);
void URIGotStatus(URI *uri, short status);
void URIGotHeader(URI *uri, struct HTTPHeader *header);
void URIGotData(URI *uri, char *data, short len);
void URIClosed(URI *uri, short error);

#endif
