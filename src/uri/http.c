#include <MacTypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "stream.h"
#include "tcpstream.h"
#include "utils.h"
#include "uri.h"
#include "http_parser.h"
#include "uri/http.h"

struct HTTPURIData {
	URI *uri;
	char *uriStr;
	Stream *tcpStream;
	http_parser parser;
	short err;
	bool closed;
};

void *HTTPProviderInit(URI *uri, char *uriStr);
void HTTPProviderClose(URI *uri, void *providerData);
void HTTPProviderRequest(URI *uri, void *providerData, HTTPMethod *method,
		Stream *postData);

struct URIProvider *httpURIProvider = &(URIProvider) {
	.init = HTTPProviderInit,
	.request = HTTPProviderRequest,
	.close = HTTPProviderClose
};

int HTTPOnMessageBegin(http_parser *parser);
int HTTPOnStatus(http_parser *parser, const char *at, size_t len);
int HTTPOnHeaderField(http_parser *parser, const char *at, size_t len);
int HTTPOnHeaderValue(http_parser *parser, const char *at, size_t len);
int HTTPOnHeadersComplete(http_parser *parser);
int HTTPOnBody(http_parser *parser, const char *at, size_t len);
int HTTPOnMessageComplete(http_parser *parser);

http_parser_settings parserSettings = {
	.on_message_begin		= HTTPOnMessageBegin,
	.on_status				= HTTPOnStatus,
	.on_header_field		= HTTPOnHeaderField,
	.on_header_value		= HTTPOnHeaderValue,
	.on_headers_complete	= HTTPOnHeadersComplete,
	.on_body				= HTTPOnBody,
	.on_message_complete	= HTTPOnMessageComplete
};

void TCPOnOpen(void *consumerData);
void TCPOnData(void *consumerData, char *data, short len);
void TCPOnError(void *consumerData, short err);
void TCPOnClose(void *consumerData);

StreamConsumer tcpConsumer = {
	.on_open = TCPOnOpen,
	.on_data = TCPOnData,
	.on_error = TCPOnError,
	.on_close = TCPOnClose,
	.on_end = TCPOnClose,
};

// create and return the provider data
void *HTTPProviderInit(URI *uri, char *uriStr)
{
	struct HTTPURIData *data;
	Stream *tcpStream = NewStream();
	if (!tcpStream) return NULL;

	data = malloc(sizeof(struct HTTPURIData));
	if (!data) {
		free(tcpStream);
		return NULL;
	}

	data->uri = uri;
	data->uriStr = url_decode(uriStr);
	data->tcpStream = tcpStream;
	data->closed = false;
	data->err = 0;
	http_parser_init(&data->parser, HTTP_RESPONSE);
	data->parser.data = data;

	StreamConsume(tcpStream, &tcpConsumer, data);

	// set up the http stream
	//ProvideTCPActiveStream(tcpStream, data->uriStr);
	ProvideTCPActiveStream(tcpStream, IP_ADDR(192,168,0,1), 80);
	return data;
}

void HTTPProviderClose(URI *uri, void *providerData)
{
	struct HTTPURIData *data = (struct HTTPURIData *)providerData;
	free(data->uriStr);
	StreamClose(data->tcpStream);
	free(data);
}

void HTTPProviderRequest(URI *uri, void *providerData, HTTPMethod *method,
		Stream *postData)
{
	struct HTTPURIData *data = (struct HTTPURIData *)providerData;

	// ignore POST data
	(void)postData;

	if (method->type != httpGET) {
		// only GET is supported
		URIClosed(uri, uriBadMethodErr);
		return;
	}

	StreamOpen(data->tcpStream);
}

// TCP connection opened
void TCPOnOpen(void *consumerData)
{
	struct HTTPURIData *hData = (struct HTTPURIData *)consumerData;
	char reqMsg[256];
	short reqLen;
	char *reqPath = "/"; // TODO
	char *host = "localhost"; // TODO

	// Build the HTTP request
	reqLen = snprintf(reqMsg, sizeof reqMsg,
			"GET %s HTTP/1.1\r\nHost: %s\r\nConnection: Close\r\n\r\n",
			reqPath, host);
	if (reqLen >= sizeof reqMsg) {
		// request was truncated
		alertf("request truncated");
		StreamClose(hData->tcpStream);
		URIClosed(hData->uri, -2);
		return;
	}

	alertf("sending http request (%hu): %s", reqLen, reqMsg);

	// Send the request
	StreamWrite(hData->tcpStream, reqMsg, reqLen);
}

void TCPOnData(void *consumerData, char *data, short len)
{
	struct HTTPURIData *hData = (struct HTTPURIData *)consumerData;
	size_t nparsed;
	alertf("tcp data [%ld]: %s", len, data);

	nparsed = http_parser_execute(&hData->parser, &parserSettings, data, len);
	if (nparsed != len) {
		// parser had an error. close connection
		StreamClose(hData->tcpStream);
		URIClosed(hData->uri, -1);
	}

}

void TCPOnError(void *consumerData, short err)
{
	struct HTTPURIData *data = (struct HTTPURIData *)consumerData;
	alertf("tcp stream error: %ld", err);
	data->err = err;
	/*
	   URIGotStatus(data->uri, status);
	   */
}

void TCPOnClose(void *consumerData)
{
	struct HTTPURIData *data = (struct HTTPURIData *)consumerData;
	if (data->closed) return;
	data->closed = true;
	URIClosed(data->uri, data->err);
}

int HTTPOnMessageBegin(http_parser *parser)
{
	return 0;
}

int HTTPOnStatus(http_parser *parser, const char *at, size_t len)
{
	struct HTTPURIData *hData = (struct HTTPURIData *)parser->data;
	URIGotStatus(hData->uri, parser->status_code);
	return 0;
}

int HTTPOnHeaderField(http_parser *parser, const char *at, size_t len)
{
	return 0;
}

int HTTPOnHeaderValue(http_parser *parser, const char *at, size_t len)
{
	return 0;
}

int HTTPOnHeadersComplete(http_parser *parser)
{
	struct HTTPURIData *hData = (struct HTTPURIData *)parser->data;
	URIHeadersComplete(hData->uri);
	return 0;
}

int HTTPOnBody(http_parser *parser, const char *at, size_t len)
{
	struct HTTPURIData *hData = (struct HTTPURIData *)parser->data;
	URIGotData(hData->uri, (char *)at, len);
	return 0;
}

int HTTPOnMessageComplete(http_parser *parser)
{
	struct HTTPURIData *hData = (struct HTTPURIData *)parser->data;
	hData->closed = true;
	URIClosed(hData->uri, hData->err);
}

