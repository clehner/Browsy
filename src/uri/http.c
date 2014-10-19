#include <MacTypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "Browsy.h"
#include "stream.h"
#include "tcpstream.h"
#include "utils.h"
#include "uri.h"
#include "http_parser.h"
#include "uri/http.h"

#define HTTP_UA "Browsy/" BROWSY_VERSION " (Macintosh; N; 68K)"
// "Lynx/2.8 (compatible; Browsy/" VERSION " (Macintosh; N; 68K)"

struct HTTPURIData {
	URI *uri;
	Stream *tcpStream;
	http_parser parser;
	short err;
	short port;
	char host[256];
	char path[256];
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
void TCPOnEnd(void *consumerData);

StreamConsumer tcpConsumer = {
	.on_open = TCPOnOpen,
	.on_data = TCPOnData,
	.on_error = TCPOnError,
	.on_close = TCPOnClose,
	.on_end = TCPOnEnd,
};

// create and return the provider data
void *HTTPProviderInit(URI *uri, char *uriStr)
{
	struct HTTPURIData *data;
	Stream *tcpStream;
	struct http_parser_url urlParser;
	size_t hostLen, pathLen;

	if (http_parser_parse_url(uriStr, strlen(uriStr), false, &urlParser)) {
		alertf("Error parsing URL %s", uriStr);
		return NULL;
	}

	if (!(urlParser.field_set & (1 << UF_HOST))) {
		alertf("URL missing host");
		return NULL;
	}
	hostLen = urlParser.field_data[UF_HOST].len;
	if (hostLen > sizeof data->host) {
		alertf("URL host too long");
		return NULL;
	}

	if (!(urlParser.field_set & (1 << UF_PATH))) {
		alertf("URL missing path");
		return NULL;
	}
	pathLen = urlParser.field_data[UF_PATH].len;
	if (!(urlParser.field_set & (1 << UF_PATH))) {
		alertf("URL missing path");
		return NULL;
	}

	data = malloc(sizeof(struct HTTPURIData));
	if (!data) {
		return NULL;
	}

	data->port = urlParser.port ? urlParser.port : 80;
	strncpy(data->host, uriStr + urlParser.field_data[UF_HOST].off, hostLen);
	strncpy(data->path, uriStr + urlParser.field_data[UF_PATH].off, pathLen);
	data->path[pathLen] = '\0';
	data->host[hostLen] = '\0';

	tcpStream = NewStream();
	if (!tcpStream) {
		free(data);
		return NULL;
	}

	data->uri = uri;
	data->tcpStream = tcpStream;
	data->err = 0;
	http_parser_init(&data->parser, HTTP_RESPONSE);
	data->parser.data = data;

	StreamConsume(tcpStream, &tcpConsumer, data);
	ProvideTCPActiveStream(tcpStream, data->host, data->port);
	return data;
}

void HTTPProviderClose(URI *uri, void *providerData)
{
	struct HTTPURIData *data = (struct HTTPURIData *)providerData;
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

	// Build the HTTP request
	reqLen = snprintf(reqMsg, sizeof reqMsg,
			"GET %s HTTP/1.1\r\n"
			"User-Agent: " HTTP_UA "\r\n"
			"Host: %s\r\n"
			"Connection: Close\r\n"
			"\r\n",
			hData->path, hData->host);
	if (reqLen >= sizeof reqMsg) {
		// request was truncated
		alertf("request truncated");
		StreamClose(hData->tcpStream);
		URIClosed(hData->uri, -2);
		return;
	}

	//alertf("sending http request (%hu): %s", reqLen, reqMsg);

	// Send the request
	StreamWrite(hData->tcpStream, reqMsg, reqLen);
}

void TCPOnData(void *consumerData, char *data, short len)
{
	struct HTTPURIData *hData = (struct HTTPURIData *)consumerData;
	size_t nparsed;

	/*
	alertf("got data (%lu)", len);
	URIGotData(hData->uri, data, len);
	return;
	*/

	nparsed = http_parser_execute(&hData->parser, &parserSettings, data, len);
	if (nparsed != len) {
		// parser had an error. close connection
		alertf("parsing error %u/%u for text (%lu): %.*s", (int)nparsed, (int)len, len, (int)len, data);
		//StreamClose(hData->tcpStream);
		//URIClosed(hData->uri, -1);
	}

}

void TCPOnError(void *consumerData, short err)
{
	struct HTTPURIData *data = (struct HTTPURIData *)consumerData;
	data->err = err;
	if (err == tcpMissingDriverErr) {
		alertf("Missing MacTCP driver");
		return;
	}
	alertf("tcp stream error: %ld", err);
	/*
	   URIGotStatus(data->uri, status);
	   */
}

void TCPOnClose(void *consumerData)
{
	struct HTTPURIData *data = (struct HTTPURIData *)consumerData;
	http_parser_execute(&data->parser, &parserSettings, "", 0);
}

void TCPOnEnd(void *consumerData)
{
}

int HTTPOnMessageBegin(http_parser *parser)
{
	struct HTTPURIData *hData = (struct HTTPURIData *)parser->data;
	URIMessageBegin(hData->uri); 
	//alertf("message begin");
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
	//TODO: check if it happens multiple times
	URIClosed(hData->uri, hData->err);
}

