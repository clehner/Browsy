#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <Memory.h>
#include <MacTypes.h>
#include "tokenizer.h"

Tokenizer *NewTokenizer() {
	Tokenizer *tok = (Tokenizer *)malloc(sizeof(Tokenizer));
	tok->state = DATA_STATE;
	return tok;
}

void DisposeTokenizer(Tokenizer *tokenizer) {
	free(tokenizer);
}

char readchar(Tokenizer *tokenizer) {

}

// Function prototypes
char *ConsumeCharacterReference(Tokenizer *tokenizer);
Boolean isAppropriateEndTagToken(TagToken *endTagToken);

#define STATE tokenizer->state

#define CASE_AZ \
	case 'A': \
	case 'B': \
	case 'C': \
	case 'D': \
	case 'E': \
	case 'F': \
	case 'G': \
	case 'H': \
	case 'I': \
	case 'J': \
	case 'K': \
	case 'L': \
	case 'M': \
	case 'N': \
	case 'O': \
	case 'P': \
	case 'Q': \
	case 'R': \
	case 'S': \
	case 'T': \
	case 'U': \
	case 'V': \
	case 'W': \
	case 'X': \
	case 'Y': \
	case 'Z'

#define CASE_az \
	case 'a': \
	case 'b': \
	case 'c': \
	case 'd': \
	case 'e': \
	case 'f': \
	case 'g': \
	case 'h': \
	case 'i': \
	case 'j': \
	case 'k': \
	case 'l': \
	case 'm': \
	case 'n': \
	case 'o': \
	case 'p': \
	case 'q': \
	case 'r': \
	case 's': \
	case 't': \
	case 'u': \
	case 'v': \
	case 'w': \
	case 'x': \
	case 'y': \
	case 'z'

#define CASE_WHITESPACE \
	case '\t': \
	case '\n': \
	case '\r': \
	case '\f': \
	case ' ' \

#define PARSE_ERROR()\
	; // TODO

#define EMIT(type, value)\
	; // TODO

#define EMIT_EOF_TOKEN\
	; // TODO

#define RECONSUME(input)\
	continue

void tokenize(Tokenizer *tokenizer) {
	TokenizerState state = tokenizer->state;
	static char replacementChar = '\xfd'; // U+FFFD
	char tempBuffer[64];
	char tempBufferL;
	TagToken *currentTagToken;
	TagToken *startTagToken;
	TagToken *endTagToken;
	char input;

	while (input = readchar(tokenizer)) {

		switch (state) {
		case DATA_STATE:
			switch(input) {
			case '&':
				STATE = CHARACTER_REFERENCE_IN_DATA_STATE;
				break;
			case '<':
				STATE = TAG_OPEN_STATE;
				break;
			case '\0':
				PARSE_ERROR();
				EMIT(CHARACTER_TOKEN, '\0');
				break;
			case EOF:
				EMIT_EOF_TOKEN;
				break;
			default:
				EMIT(CHARACTER_TOKEN, input);
				break;
			}
		break;
		case CHARACTER_REFERENCE_IN_DATA_STATE:
		{
			char *charRefs = ConsumeCharacterReference(tokenizer);
			if (charRefs) {
				EMIT(CHARACTER_TOKENS, charRefs);
			} else {
				EMIT(CHARACTER_TOKEN, '&');
			}
		}
		break;
		case RCDATA_STATE:
			switch(input) {
			case '&':
				STATE = CHARACTER_REFERENCE_IN_RCDATA_STATE;
				break;
			case '<':
				STATE = RCDATA_LESS_THAN_SIGN_STATE;
				break;
			case '\0':
				PARSE_ERROR();
				EMIT(CHARACTER_TOKEN, replacementChar);
				break;
			case EOF:
				EMIT_EOF_TOKEN;
				break;
			default:
				EMIT(CHARACTER_TOKEN, input);
				break;
			}
		break;
		case CHARACTER_REFERENCE_IN_RCDATA_STATE:
		{
			char *charRefs = ConsumeCharacterReference(tokenizer);
			if (charRefs) {
				EMIT(CHARACTER_TOKENS, charRefs);
			} else {
				EMIT(CHARACTER_TOKEN, '&');
			}
		}
		break;
		case RAWTEXT_STATE:
			switch(input) {
			case '<':
				STATE = RAWTEXT_LESS_THAN_SIGN_STATE;
				break;
			case '\0':
				PARSE_ERROR();
				EMIT(CHARACTER_TOKEN, replacementChar);
				break;
			case EOF:
				EMIT_EOF_TOKEN;
				break;
			default:
				EMIT(CHARACTER_TOKEN, input);
				break;
			}
		break;
		case SCRIPT_DATA_STATE:
			switch(input) {
			case '<':
				STATE = SCRIPT_DATA_LESS_THAN_SIGN_STATE;
				break;
			case '\0':
				PARSE_ERROR();
				EMIT(CHARACTER_TOKEN, replacementChar);
				break;
			case EOF:
				EMIT_EOF_TOKEN;
				break;
			default:
				EMIT(CHARACTER_TOKEN, input);
				break;
			}
		break;
		case PLAINTEXT_STATE:
			switch(input) {
			case '\0':
				PARSE_ERROR();
				EMIT(CHARACTER_TOKEN, replacementChar);
				break;
			case EOF:
				EMIT_EOF_TOKEN;
				break;
			default:
				EMIT(CHARACTER_TOKEN, input);
				break;
			}
		break;
		case TAG_OPEN_STATE:
			switch(input) {
			case '!':
				STATE = MARKUP_DECLARATION_OPEN_STATE;
				break;
			case '/':
				STATE = END_TAG_OPEN_STATE;
				break;
			CASE_AZ:
				// new startTagToken
				startTagToken->tagName[0] = tolower(input);
				startTagToken->tagName[1] = '\0';
				startTagToken->tagNameLength = 1;
				STATE = TAG_NAME_STATE;
				break;
			CASE_az:
				// new startTagToken
				startTagToken->tagName[0] = input;
				startTagToken->tagName[1] = '\0';
				startTagToken->tagNameLength = 1;
				STATE = TAG_NAME_STATE;
				break;
			case '?':
				PARSE_ERROR();
				STATE = BOGUS_COMMENT_STATE;
				break;
			default:
				PARSE_ERROR();
				STATE = DATA_STATE;
				EMIT(CHARACTER_TOKEN, '<');
				RECONSUME(input);
				break;
			}
		break;
		case END_TAG_OPEN_STATE:
			switch(input) {
			CASE_AZ:
				// new startTagToken
				endTagToken->tagName[0] = tolower(input);
				endTagToken->tagName[1] = '\0';
				endTagToken->tagNameLength = 1;
				STATE = TAG_NAME_STATE;
				break;
			CASE_az:
				// new startTagToken
				endTagToken->tagName[0] = input;
				endTagToken->tagName[1] = '\0';
				endTagToken->tagNameLength = 1;
				STATE = TAG_NAME_STATE;
				break;
			case '>':
				PARSE_ERROR();
				STATE = DATA_STATE;
				break;
			case EOF:
				PARSE_ERROR();
				STATE = DATA_STATE;
				EMIT(CHARACTER_TOKENS, "</");
				RECONSUME(input);
				break;
			default:
				PARSE_ERROR();
				STATE = BOGUS_COMMENT_STATE;
				break;
			}
		break;
		case TAG_NAME_STATE:
			switch(input) {
			CASE_WHITESPACE:
				STATE = BEFORE_ATTRIBUTE_NAME_STATE;
				break;
			case '/':
				STATE = SELF_CLOSING_START_TAG_STATE;
				break;
			case '>':
				STATE = DATA_STATE;
				EMIT(CHARACTER_TOKEN, input);
				break;
			CASE_AZ:
				if (currentTagToken->tagNameLength+1 <
						sizeof(currentTagToken->tagName)) {
					currentTagToken->tagName[currentTagToken->tagNameLength++] =
						tolower(input);
				}
				break;
			case '\0':
				PARSE_ERROR();
				EMIT(CHARACTER_TOKEN, replacementChar);
				break;
			case EOF:
				PARSE_ERROR();
				STATE = DATA_STATE;
				RECONSUME(input);
				break;
			default:
				if (currentTagToken->tagNameLength+1 <
						sizeof(currentTagToken->tagName)) {
					currentTagToken->tagName[currentTagToken->tagNameLength++] =
						input;
				}
			}
		break;
		case RCDATA_LESS_THAN_SIGN_STATE:
			switch(input) {
			case '/':
				tempBuffer[0] = '\0';
				tempBufferL = 0;
				STATE = RCDATA_END_TAG_OPEN_STATE;
				break;
			default:
				STATE = RCDATA_STATE;
				EMIT(CHARACTER_TOKEN, '<');
				RECONSUME(input);
			}
		break;
		case RCDATA_END_TAG_OPEN_STATE:
			switch(input) {
			CASE_AZ:
				endTagToken->tagName[0] = tolower(input);
				endTagToken->tagName[1] = '\0';
				endTagToken->tagNameLength = 1;
				if (tempBufferL+1 < sizeof(tempBuffer)) {
					tempBuffer[tempBufferL++] = input;
				}
				STATE = RCDATA_END_TAG_NAME_STATE;
				break;
			CASE_az:
				endTagToken->tagName[0] = input;
				endTagToken->tagName[1] = '\0';
				endTagToken->tagNameLength = 1;
				if (tempBufferL+1 < sizeof(tempBuffer)) {
					tempBuffer[tempBufferL++] = input;
				}
				STATE = RCDATA_END_TAG_NAME_STATE;
				break;
			default:
				STATE = RCDATA_STATE;
				EMIT(CHARACTER_TOKENS, "</");
				RECONSUME(input);
			}
		break;
		case RCDATA_END_TAG_NAME_STATE:
			switch(input) {
			CASE_WHITESPACE:
				if (isAppropriateEndTagToken(endTagToken)) {
					STATE = BEFORE_ATTRIBUTE_NAME_STATE;
				} else {
					goto nowayjose;
				}
				break;
			case '/':
				if (isAppropriateEndTagToken(endTagToken)) {
					STATE = SELF_CLOSING_START_TAG_STATE;
				} else {
					goto nowayjose;
				}
				break;
			case '>':
				if (isAppropriateEndTagToken(endTagToken)) {
					STATE = DATA_STATE;
					EMIT(TAG_TOKEN, endTagToken);
				} else {
					goto nowayjose;
				}
				break;
			CASE_AZ:
				if (endTagToken->tagNameLength+1 <
						sizeof(endTagToken->tagName)) {
					endTagToken->tagName[endTagToken->tagNameLength++] =
						tolower(input);
				}
				if (tempBufferL+1 < sizeof(tempBuffer)) {
					tempBuffer[tempBufferL++] = input;
				}
				break;
			CASE_az:
				if (endTagToken->tagNameLength+1 <
						sizeof(endTagToken->tagName)) {
					endTagToken->tagName[endTagToken->tagNameLength++] =
						input;
				}
				if (tempBufferL+1 < sizeof(tempBuffer)) {
					tempBuffer[tempBufferL++] = input;
				}
				break;
			default:
				STATE = RCDATA_STATE;
				EMIT(CHARACTER_TOKENS, "</");
				EMIT(CHARACTER_TOKENS, tempBuffer);
				RECONSUME(input);
			}
		break;
		case RAWTEXT_LESS_THAN_SIGN_STATE:
			switch(input) {
			case '/':
				tempBuffer[0] = '\0';
				tempBufferL = 0;
				STATE = RAWTEXT_END_TAG_OPEN_STATE;
				break;
			default:
				STATE = RAWTEXT_STATE;
				EMIT(CHARACTER_TOKEN, '<');
				RECONSUME(input);
			}
		break;
		case RAWTEXT_END_TAG_OPEN_STATE:
			switch(input) {
			CASE_AZ:
				// new endTagToken
				endTagToken->tagName[0] = tolower(input);
				endTagToken->tagName[1] = '\0';
				endTagToken->tagNameLength = 1;
				if (tempBufferL+1 < sizeof(tempBuffer)) {
					tempBuffer[tempBufferL++] = input;
				}
				STATE = RAWTEXT_END_TAG_NAME_STATE;
				break;
			CASE_az:
				// new endTagToken
				endTagToken->tagName[0] = input;
				endTagToken->tagName[1] = '\0';
				endTagToken->tagNameLength = 1;
				if (tempBufferL+1 < sizeof(tempBuffer)) {
					tempBuffer[tempBufferL++] = input;
				}
				break;
			default:
				STATE = RAWTEXT_STATE;
				EMIT(CHARACTER_TOKENS, "</");
				RECONSUME(input);
			}
		break;
		case RAWTEXT_END_TAG_NAME_STATE:
			switch(input) {
			CASE_WHITESPACE:
				if (isAppropriateEndTagToken(endTagToken)) {
					STATE = BEFORE_ATTRIBUTE_NAME_STATE;
				} else {
					goto nowayjose;
				}
				break;
			case '/':
				if (isAppropriateEndTagToken(endTagToken)) {
					STATE = SELF_CLOSING_START_TAG_STATE;
				} else {
					goto nowayjose;
				}
				break;
			case '>':
				if (isAppropriateEndTagToken(endTagToken)) {
					STATE = DATA_STATE;
				} else {
					goto nowayjose;
				}
				break;
			CASE_AZ:
				if (endTagToken->tagNameLength+1 <
						sizeof(endTagToken->tagName)) {
					endTagToken->tagName[endTagToken->tagNameLength++] =
						tolower(input);
				}
				if (tempBufferL+1 < sizeof(tempBuffer)) {
					tempBuffer[tempBufferL++] = input;
				}
				break;
			CASE_az:
				if (endTagToken->tagNameLength+1 <
						sizeof(endTagToken->tagName)) {
					endTagToken->tagName[endTagToken->tagNameLength++] =
						input;
				}
				if (tempBufferL+1 < sizeof(tempBuffer)) {
					tempBuffer[tempBufferL++] = input;
				}
				break;
			default:
			nowayjose:
				STATE = RAWTEXT_STATE;
				EMIT(CHARACTER_TOKENS, "</");
				EMIT(CHARACTER_TOKENS, tempBuffer);
				RECONSUME(input);
			}
		break;
		/*
		case SCRIPT_DATA_LESS_THAN_SIGN_STATE:
			switch(input) {
			case '':
				break;
			case EOF:
				break;
			default:
				break;
			}
		break;
		case SCRIPT_DATA_END_TAG_OPEN_STATE:
			switch(input) {
			case '':
				break;
			case EOF:
				break;
			default:
				break;
			}
		break;
		case SCRIPT_DATA_END_TAG_NAME_STATE:
			switch(input) {
			case '':
				break;
			case EOF:
				break;
			default:
				break;
			}
		break;
		case SCRIPT_DATA_ESCAPE_START_STATE:
			switch(input) {
			case '':
				break;
			case EOF:
				break;
			default:
				break;
			}
		break;
		case SCRIPT_DATA_ESCAPE_START_DASH_STATE:
			switch(input) {
			case '':
				break;
			case EOF:
				break;
			default:
				break;
			}
		break;
		case SCRIPT_DATA_ESCAPED_STATE:
			switch(input) {
			case '':
				break;
			case EOF:
				break;
			default:
				break;
			}
		break;
		case SCRIPT_DATA_ESCAPED_DASH_STATE:
			switch(input) {
			case '':
				break;
			case EOF:
				break;
			default:
				break;
			}
		break;
		case SCRIPT_DATA_ESCAPED_DASH_DASH_STATE:
			switch(input) {
			case '':
				break;
			case EOF:
				break;
			default:
				break;
			}
		break;
		case SCRIPT_DATA_ESCAPED_LESS_THAN_SIGN_STATE:
			switch(input) {
			case '':
				break;
			case EOF:
				break;
			default:
				break;
			}
		break;
		case SCRIPT_DATA_ESCAPED_END_TAG_OPEN_STATE:
			switch(input) {
			case '':
				break;
			case EOF:
				break;
			default:
				break;
			}
		break;
		case SCRIPT_DATA_ESCAPED_END_TAG_NAME_STATE:
			switch(input) {
			case '':
				break;
			case EOF:
				break;
			default:
				break;
			}
		break;
		case SCRIPT_DATA_DOUBLE_ESCAPE_START_STATE:
			switch(input) {
			case '':
				break;
			case EOF:
				break;
			default:
				break;
			}
		break;
		case SCRIPT_DATA_DOUBLE_ESCAPED_STATE:
			switch(input) {
			case '':
				break;
			case EOF:
				break;
			default:
				break;
			}
		break;
		case SCRIPT_DATA_DOUBLE_ESCAPED_DASH_STATE:
			switch(input) {
			case '':
				break;
			case EOF:
				break;
			default:
				break;
			}
		break;
		case SCRIPT_DATA_DOUBLE_ESCAPED_DASH_DASH_STATE:
			switch(input) {
			case '':
				break;
			case EOF:
				break;
			default:
				break;
			}
		break;
		case SCRIPT_DATA_DOUBLE_ESCAPED_LESS_THAN_SIGN_STATE:
			switch(input) {
			case '':
				break;
			case EOF:
				break;
			default:
				break;
			}
		break;
		case SCRIPT_DATA_DOUBLE_ESCAPE_END_STATE:
			switch(input) {
			case '':
				break;
			case EOF:
				break;
			default:
				break;
			}
		break;
		case BEFORE_ATTRIBUTE_NAME_STATE:
			switch(input) {
			case '':
				break;
			case EOF:
				break;
			default:
				break;
			}
		break;
		case ATTRIBUTE_NAME_STATE:
			switch(input) {
			case '':
				break;
			case EOF:
				break;
			default:
				break;
			}
		break;
		case AFTER_ATTRIBUTE_NAME_STATE:
			switch(input) {
			case '':
				break;
			case EOF:
				break;
			default:
				break;
			}
		break;
		case BEFORE_ATTRIBUTE_VALUE_STATE:
			switch(input) {
			case '':
				break;
			case EOF:
				break;
			default:
				break;
			}
		break;
		case ATTRIBUTE_VALUE_DOUBLE_QUOTED_STATE:
			switch(input) {
			case '':
				break;
			case EOF:
				break;
			default:
				break;
			}
		break;
		case ATTRIBUTE_VALUE_SINGLE_QUOTED_STATE:
			switch(input) {
			case '':
				break;
			case EOF:
				break;
			default:
				break;
			}
		break;
		case ATTRIBUTE_VALUE_UNQUOTED_STATE:
			switch(input) {
			case '':
				break;
			case EOF:
				break;
			default:
				break;
			}
		break;
		case CHARACTER_REFERENCE_IN_ATTRIBUTE_VALUE_STATE:
			switch(input) {
			case '':
				break;
			case EOF:
				break;
			default:
				break;
			}
		break;
		case AFTER_ATTRIBUTE_VALUE_QUOTED_STATE:
			switch(input) {
			case '':
				break;
			case EOF:
				break;
			default:
				break;
			}
		break;
		case SELF_CLOSING_START_TAG_STATE:
			switch(input) {
			case '':
				break;
			case EOF:
				break;
			default:
				break;
			}
		break;
		case BOGUS_COMMENT_STATE:
			switch(input) {
			case '':
				break;
			case EOF:
				break;
			default:
				break;
			}
		break;
		case MARKUP_DECLARATION_OPEN_STATE:
			switch(input) {
			case '':
				break;
			case EOF:
				break;
			default:
				break;
			}
		break;
		case COMMENT_START_STATE:
			switch(input) {
			case '':
				break;
			case EOF:
				break;
			default:
				break;
			}
		break;
		case COMMENT_START_DASH_STATE:
			switch(input) {
			case '':
				break;
			case EOF:
				break;
			default:
				break;
			}
		break;
		case COMMENT_STATE:
			switch(input) {
			case '':
				break;
			case EOF:
				break;
			default:
				break;
			}
		break;
		case COMMENT_END_DASH_STATE:
			switch(input) {
			case '':
				break;
			case EOF:
				break;
			default:
				break;
			}
		break;
		case COMMENT_END_STATE:
			switch(input) {
			case '':
				break;
			case EOF:
				break;
			default:
				break;
			}
		break;
		case COMMENT_END_BANG_STATE:
			switch(input) {
			case '':
				break;
			case EOF:
				break;
			default:
				break;
			}
		break;
		case DOCTYPE_STATE:
			switch(input) {
			case '':
				break;
			case EOF:
				break;
			default:
				break;
			}
		break;
		case BEFORE_DOCTYPE_NAME_STATE:
			switch(input) {
			case '':
				break;
			case EOF:
				break;
			default:
				break;
			}
		break;
		case DOCTYPE_NAME_STATE:
			switch(input) {
			case '':
				break;
			case EOF:
				break;
			default:
				break;
			}
		break;
		case AFTER_DOCTYPE_NAME_STATE:
			switch(input) {
			case '':
				break;
			case EOF:
				break;
			default:
				break;
			}
		break;
		case AFTER_DOCTYPE_PUBLIC_KEYWORD_STATE:
			switch(input) {
			case '':
				break;
			case EOF:
				break;
			default:
				break;
			}
		break;
		case BEFORE_DOCTYPE_PUBLIC_IDENTIFIER_STATE:
			switch(input) {
			case '':
				break;
			case EOF:
				break;
			default:
				break;
			}
		break;
		case DOCTYPE_PUBLIC_IDENTIFIER_DOUBLE_QUOTED_STATE:
			switch(input) {
			case '':
				break;
			case EOF:
				break;
			default:
				break;
			}
		break;
		case DOCTYPE_PUBLIC_IDENTIFIER_SINGLE_QUOTED_STATE:
			switch(input) {
			case '':
				break;
			case EOF:
				break;
			default:
				break;
			}
		break;
		case AFTER_DOCTYPE_PUBLIC_IDENTIFIER_STATE:
			switch(input) {
			case '':
				break;
			case EOF:
				break;
			default:
				break;
			}
		break;
		case BETWEEN_DOCTYPE_PUBLIC_AND_SYSTEM_IDENTIFIERS_STATE:
			switch(input) {
			case '':
				break;
			case EOF:
				break;
			default:
				break;
			}
		break;
		case AFTER_DOCTYPE_SYSTEM_KEYWORD_STATE:
			switch(input) {
			case '':
				break;
			case EOF:
				break;
			default:
				break;
			}
		break;
		case BEFORE_DOCTYPE_SYSTEM_IDENTIFIER_STATE:
			switch(input) {
			case '':
				break;
			case EOF:
				break;
			default:
				break;
			}
		break;
		case DOCTYPE_SYSTEM_IDENTIFIER_DOUBLE_QUOTED_STATE:
			switch(input) {
			case '':
				break;
			case EOF:
				break;
			default:
				break;
			}
		break;
		case DOCTYPE_SYSTEM_IDENTIFIER_SINGLE_QUOTED_STATE:
			switch(input) {
			case '':
				break;
			case EOF:
				break;
			default:
				break;
			}
		break;
		case AFTER_DOCTYPE_SYSTEM_IDENTIFIER_STATE:
			switch(input) {
			case '':
				break;
			case EOF:
				break;
			default:
				break;
			}
		break;
		case BOGUS_DOCTYPE_STATE:
			switch(input) {
			case '':
				break;
			case EOF:
				break;
			default:
				break;
			}
		break;
		case CDATA_SECTION_STATE:
			switch(input) {
			case '':
				break;
			}
		break;
		*/
		default:
			// State not yet implemented
			return;
		}
	}
}

char *ConsumeCharacterReference(Tokenizer *tokenizer) {
	//http://whatwg.org/specs/web-apps/current-work/multipage/tokenization.html#consume-a-character-reference
}

Boolean isAppropriateEndTagToken(TagToken *endTagToken) {
}
