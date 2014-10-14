#include <MacTypes.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "stream.h"
#include "filestream.h"
#include "utils.h"
#include "uri.h"
#include "uri/file.h"

struct FileURIData {
	URI *uri;
	char *filePath;
	Stream *fileStream;
	short err;
	bool closed;
};

void *FileProviderInit(URI *uri, char *uriStr);
void FileProviderClose(URI *uri, void *providerData);
void FileProviderRequest(URI *uri, void *providerData, HTTPMethod *method,
		Stream *postData);

struct URIProvider *fileURIProvider = &(URIProvider) {
	.init = FileProviderInit,
	.request = FileProviderRequest,
	.close = FileProviderClose
};

void FileConsumeOpen(void *consumerData);
void FileConsumeData(void *consumerData, char *data, short len);
void FileConsumeError(void *consumerData, short err);
void FileConsumeClose(void *consumerData);

StreamConsumer consumer = {
	.on_open = FileConsumeOpen,
	.on_data = FileConsumeData,
	.on_error = FileConsumeError,
	.on_close = FileConsumeClose,
	.on_end = FileConsumeClose,
};

// create and return the provider data
void *FileProviderInit(URI *uri, char *uriStr)
{
	char *pathStart, *path;
	short len;
	struct FileURIData *data;
	Stream *fileStream = NewStream();
	if (!fileStream) return NULL;

	data = malloc(sizeof(struct FileURIData));
	if (!data) {
		free(fileStream);
		return NULL;
	}

	data->uri = uri;
	data->fileStream = fileStream;
	data->closed = false;
	data->err = 0;
	StreamConsume(fileStream, &consumer, data);

	// skip "file:"
	uriStr += 5;
	data->filePath = url_decode(uriStr);
	// skip initial slashes
	for (pathStart = data->filePath; pathStart[0] == '/'; pathStart++);
	// replace / with :
	len = 0;
	for (path = pathStart; *path; path++) {
		len++;
		switch (*path) {
			case '/': *path = ':'; break;
			case ':': *path = '/';
		}
	}

	// make into pascal string
	*--pathStart = len;

	// set up the file stream
	ProvideFileStream(fileStream, pathStart, 0);

	return data;
}

void FileProviderClose(URI *uri, void *providerData)
{
	struct FileURIData *data = (struct FileURIData *)providerData;
	free(data->filePath);
	free(data);
}

void FileProviderRequest(URI *uri, void *providerData, HTTPMethod *method,
		Stream *postData)
{
	struct FileURIData *data = (struct FileURIData *)providerData;

	// ignore POST data
	(void)postData;

	if (method->type != httpGET) {
		// only GET is supported
		URIClosed(uri, uriBadMethodErr);
		return;
	}

	StreamOpen(data->fileStream);
}

void FileConsumeOpen(void *consumerData)
{
	struct FileURIData *data = (struct FileURIData *)consumerData;
	URIGotStatus(data->uri, 200);
}

void FileConsumeData(void *consumerData, char *data, short len)
{
	struct FileURIData *fData = (struct FileURIData *)consumerData;
	URIGotData(fData->uri, data, len);
}

void FileConsumeError(void *consumerData, short err)
{
	struct FileURIData *data = (struct FileURIData *)consumerData;
	short status = 0;
	alertf("file stream error: %ld", err);
	data->err = err;
	if (err == fnfErr || err == nsvErr) {
		status = 404;
	}
	if (status) {
		URIGotStatus(data->uri, status);
	}
}

void FileConsumeClose(void *consumerData)
{
	struct FileURIData *data = (struct FileURIData *)consumerData;
	if (data->closed) return;
	data->closed = true;
	URIClosed(data->uri, data->err);
}

