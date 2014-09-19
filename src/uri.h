typedef enum {
	stateResolving,
	stateConnecting,
	stateLoading,
	stateComplete
} URIRequestState;

enum {
	methodGET,
	methodPOST
};

/*
enum {
	err
}
*/

typedef struct HTTPHeader {
	char *name;
	char *value;
	struct HTTPHeader *next;
} HTTPHeader;

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

URIResponse *NewResponse();
void DisposeResponse(URIResponse *resp);
void DisposeHeaders(HTTPHeader *header);
void DisposeRequest(URIRequest *req);
void RequestURI(char *uri, void (*callback)(URIRequest *req), void *refCon);
