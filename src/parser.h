#ifndef _PARSER_H
#define _PARSER_H

typedef enum {
	ParserStateStart,
} ParserState;

typedef struct {
	ParserState state;
} Parser;

#endif
