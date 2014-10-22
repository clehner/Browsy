#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <StandardFile.h>
#include <StringCompare.h>
#include <TextEdit.h>
#include <Traps.h>
#include "Browsy.h"
#include "utils.h"

static Boolean HasWNE;

void CheckWNE() {
	HasWNE = (GetTrapAddress(_WaitNextEvent) !=
			  GetTrapAddress(_Unimplemented));
}

// wait/get next event
Boolean WNE(EventMask eventMask, EventRecord *theEvent, UInt32 sleep, RgnHandle mouseRgn) {
	if (HasWNE) {
		return WaitNextEvent(eventMask, theEvent, sleep, mouseRgn);
	} else {
		// Single Finder, System 6 or less
		// Give time to desk accessories
		SystemTask();
		return GetNextEvent(eventMask, theEvent);
	}
}

// check equality of pascal strings (case insensitive)
Boolean EqualPStringCase(ConstStr255Param str1, ConstStr255Param str2) {
    return !strncasecmp(str1, str2, str1[0]+1);
}

// GetFilePathName and GetFilePathVolRef are from MacTech:
// www.mactech.com/articles/mactech/Vol.07/07.09/FilePath/

char *GetFilePathName(vRefNum, fName)
 int vRefNum;              /* File's vol/dir ref    */
 Str255 fName;             /* (pascal string) */
 {
  WDPBRec      wDir;         /* Working directory     */
  HVolumeParam wVol;         /* Working HFS param blk */
  DirInfo      wCInfo;       /* Working cat info blk  */
  long         wDirID;       /* Working dir number    */
  Str255       wName;        /* Working directory name*/
  char         *wPtr;        /* Working string pointer*/
  long         wLength;      /* Working string length */
  char         *pathFileName;/* Working file path name*/
  short i;

  wDir.ioNamePtr = 0L;       /* Init working directory*/
  wDir.ioVRefNum = vRefNum;
  wDir.ioWDIndex = 0;
  wDir.ioWDProcID = 0;
  wDir.ioWDVRefNum = vRefNum;

  (void) PBGetWDInfo(&wDir,FALSE);  /* Get the directory ref */

  vRefNum = wDir.ioWDVRefNum;/* Save working vol ref #*/
  wDirID = wDir.ioWDDirID;   /* Save working dir ref #*/

  wVol.ioNamePtr = (StringPtr)&wName;/* Init vol block*/
  wVol.ioVRefNum = vRefNum;
  wVol.ioVolIndex = 0;

  wLength = 0L;              /* Set path length to zip*/
  pathFileName = NewPtr(0L); /* Set null file's path  */

   if (!PBHGetVInfoSync((HParmBlkPtr)&wVol) &&/* Got vol info?    */
   pathFileName) {           /* Got file path pointer?*/
   if (wVol.ioVSigWord == 0x4244) {/* Check if it HFS */
    wCInfo.ioNamePtr = (StringPtr)&wName;/* Init it   */
    wCInfo.ioVRefNum = vRefNum;
    wCInfo.ioFDirIndex = -1;
    wCInfo.ioDrParID = wDirID;
    wCInfo.ioDrDirID = wDirID;

    while (wCInfo.ioDrParID != 1){/* Do full path     */
     wCInfo.ioDrDirID = wCInfo.ioDrParID;/*Move up dir*/

     if (PBGetCatInfo((CInfoPBPtr)&wCInfo,FALSE))/* Get dir info  */
      break;                 /* Break-out if failed!! */

     wLength += wName[0] + 1L;/* Set string length    */
     wPtr = NewPtr(wLength + 1L);/* Alloc new str     */

     if (!wPtr) {            /* Didn't get str ptr?   */
      if (pathFileName)      /* Check if got memory   */
       DisposePtr(pathFileName);/* Release it          */
      pathFileName = 0L;     /* Invalidate str pointer*/
      break;                 /* Break-out if failed!! */
     }
                             /* Shuffle file path down*/
     BlockMove(pathFileName,wPtr + wName[0] + 1,
               wLength - (long)(wName[0]));
     DisposePtr(pathFileName);/* Release old one       */
     *(wPtr + wName[0]) = ':';/* Add dir delimeter    */
     BlockMove(&wName[1],pathFileName = wPtr, (long)(wName[0]));
    }
   }
   else {                    /* Oops, get vol info    */
    wLength = wName[0] + 1L; /* Set string length     */
    wPtr = NewPtr(wLength + 1L);/* Alloc new string   */

    if (wPtr) {              /* Got string pointer?   */
     *(wPtr + wName[0]) = ':';/* Tack on dir delimeter*/
     BlockMove(&wName[1],pathFileName = wPtr,
               (long)(wName[0]));
    }

   }
   if (pathFileName)         /* Check if got da string*/
    pathFileName[wLength] = 0;/* Set end-of-string    */
  }

  // append filename
  //strncpy(pathFileName + wLength, fName+1, fName[0]);
  wPtr = NewPtr(wLength + fName[0] + 1);
  BlockMove(pathFileName, wPtr, wLength);
  DisposePtr(pathFileName);
  BlockMove(&fName[1], (pathFileName = wPtr) + wLength, (long)(fName[0]));
  wLength += fName[0];
  pathFileName[wLength] = 0;

  // convert path to unix-style
  for (i=0; i<wLength; i++) {
   if (pathFileName[i] == '/') {
    pathFileName[i] = ':';
   } else if (pathFileName[i] == ':') {
    pathFileName[i] = '/';
   }
  }
  return(pathFileName);      /* Return file path name */
 }

int GetFilePathVolRef(pathFileName)
 char *pathFileName;         /* File's path string    */
 {
  short        i;            /* Working index         */
  char         c;            /* Working input char    */
  int        vRefNum;      /* Working vol/dir ref   */
  WDPBRec      wDir;         /* Working directory     */
  HVolumeParam wVol;         /* Working HFS param blk */
  DirInfo      wCInfo;       /* Working cat info block*/
  //long         wDirID;       /* Working directory ID  */
  Str255       wVolName;     /* Working volume name   */
  Str255       wName;        /* Working string name   */
  //char         *wPtr;        /* Working string pointer*/
  short        wLength;      /* Working string length */

  // simplify union type casting

  vRefNum = 0;               /* Invalid vol/dir ref # */

  wVol.ioVRefNum = 0;        /* Init working vol ID # */
  wCInfo.ioDrDirID = 2;      /* Init top-most dir     */

  i = 0;                     /* Init working index    */
  wLength = 0;               /* Init string length    */

  c = pathFileName[i];       /* Set 1st input char    */

  while(c) {                 /* Do til get vol/dir ref*/
   if (c == '/') {           /* Check if got dir specs*/
    wName[0] = wLength - 1;  /* Set "Pascal" str len  */
    wLength = 0;             /* Reset string length   */

    if (!wVol.ioVRefNum) {   /* Check if need vol ref */
     wVol.ioNamePtr = (StringPtr)(&wVolName);

     for(wVol.ioVolIndex = 1;
       !PBHGetVInfoSync((HParmBlkPtr)&wVol);
       wVol.ioVolIndex++) {
      if (EqualPStringCase(wName,wVolName))
       break;
      }

     vRefNum = wVol.ioVRefNum;/* Save vol ID          */
     if (wVol.ioVSigWord != 0x4244)/* MFS ?           */
      return(vRefNum);       /* Let's get out early!  */
    }
    else {                   /* Nope, get dir ref     */
     wCInfo.ioNamePtr = (StringPtr)(&wName);
     wCInfo.ioVRefNum = wVol.ioVRefNum;
     wCInfo.ioFDirIndex = 0;
     wCInfo.ioDrParID = wCInfo.ioDrDirID;

     if (PBGetCatInfo((CInfoPBPtr)&wCInfo,FALSE))/* Check dir ?   */
      return(0);             /* Nope, break-out!!!    */
    }
   }                         /* Add to directory specs*/
   c = wName[++wLength] = pathFileName[i++];
   if (c == ':') c = wName[wLength] = '/';
  }
  wDir.ioNamePtr = 0L;       /* Init dir data block   */
  wDir.ioVRefNum = wVol.ioVRefNum;
  wDir.ioWDProcID = 'ERIK';  /* Magic 'SFGetFile' ID #*/
  wDir.ioWDDirID = wCInfo.ioDrDirID;

  if (!PBOpenWD(&wDir,FALSE))/* Check if opened dir   */
   vRefNum = wDir.ioVRefNum; /* Return vol/dir ref #  */

  return(vRefNum);           /* Return vol/dir ref #  */
 }


// Get the filename of the file in a path. (c strings)
const char *getFilePathFileName(const char * pathFileName)
{
    const char *fName = strrchr(pathFileName, '/');
    return fName ? fName + 1 : pathFileName;
}

// URL Encode/decode
// from http://www.geekhideout.com/urlencode.shtml
// with modifications

 /* Converts a hex character to its integer value */
char from_hex(char ch) {
  return isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
}

/* Converts an integer value to its hex character*/
char to_hex(char code) {
  static char hex[] = "0123456789ABCDEF";
  return hex[code & 15];
}

/* Returns a url-encoded version of str */
/* IMPORTANT: be sure to free() the returned string after use */
char *url_encode(char *str) {
  char *pstr = str, *buf = (char*)malloc(strlen(str) * 3 + 1), *pbuf = buf;
  if (!buf) return NULL;
  while (*pstr) {
    if (isalnum(*pstr) ||
	  *pstr == '-' || *pstr == '_' || *pstr == '.' || *pstr == '~' ||
	  *pstr == '/')
	  //*pstr == ':' || *pstr == '/' || *pstr == '?' || *pstr == '&' ||
	  //*pstr == '=')
      *pbuf++ = *pstr;
    //else if (*pstr == ' ')
      //*pbuf++ = '+';
    else
      *pbuf++ = '%', *pbuf++ = to_hex(*pstr >> 4), *pbuf++ = to_hex(*pstr & 15);
    pstr++;
  }
  *pbuf = '\0';
  return buf;
}

/* Returns a url-decoded version of str */
/* IMPORTANT: be sure to free() the returned string after use */
char *url_decode(char *str) {
  char *pstr = str, *buf = (char*)malloc(strlen(str) + 1), *pbuf = buf;
  if (!buf) return NULL;
  while (*pstr) {
    if (*pstr == '%') {
      if (pstr[1] && pstr[2]) {
        *pbuf++ = from_hex(pstr[1]) << 4 | from_hex(pstr[2]);
        pstr += 2;
      }
    //} else if (*pstr == '+') {
      //*pbuf++ = ' ';
    } else {
      *pbuf++ = *pstr;
    }
    pstr++;
  }
  *pbuf = '\0';
  return buf;
}

// get where a character is safe for url
// return true if it is safe
// return false if it has to be escaped
static inline char is_char_urlsafe(char c) {
	return isalnum(c) ||
		c == '-' || c == '_' || c == '.' || c == '~' ||
		c == ':' || c == '/' || c == '?' || c == '&' ||
		c == '=' || c == '%' || c == ';';
}

/* Make sure a str is url-encoded and has a scheme */
/* IMPORTANT: be sure to free() the returned string after use */
char *url_sanitize(char *str) {
	static const char default_scheme[] = "http://";
	char insert_default_scheme = true;
	short len = strlen(str) + 1;
	char pchar;
	char *pstr, *buf, *pbuf;

	// calculate new string length
	for (pstr = str; pchar = *pstr; pstr++) {
		if (!is_char_urlsafe(pchar))
			len += 2;
		else if (pchar == ':' && insert_default_scheme) {
			insert_default_scheme = false;
		}
	}
	if (insert_default_scheme) {
		len += sizeof(default_scheme) - 1;
	}

	// allocate the string
	buf = pbuf = malloc(len);
	if (!buf) return NULL;

	// copy the string
	if (insert_default_scheme) {
		strcpy(pbuf, default_scheme);
		pbuf += sizeof(default_scheme) - 1;
	}
	for (pstr = str; pchar = *pstr; pstr++) {
		if (is_char_urlsafe(pchar)) {
			*pbuf++ = pchar;
		} else {
			*pbuf++ = '%';
			*pbuf++ = to_hex(pchar >> 4);
			*pbuf++ = to_hex(pchar & 15);
		}
	}
	*pbuf = '\0';
	return buf;
}

// from Apple Technical Note TN1019,
// with modifications for changing the source mode (removed)
void PlotSICN(Rect *theRect, SICNHand theSICN, long theIndex/*, short srcMode*/) {
	char	state;		/* saves original flags of 'SICN' handle */
	BitMap	srcBits;	/* built up around 'SICN' data so we can CopyBits */
	short srcMode = srcCopy;
	//if (!srcMode) srcMode = srcCopy;

	/* check the index for a valid value */
	if ((InlineGetHandleSize((Handle)theSICN) / sizeof(SICN)) > theIndex) {

		/* store the resource's current locked/unlocked condition */
		state = HGetState((Handle)theSICN);

		/* lock the resource so it won't move during the CopyBits call */
		HLock((Handle)theSICN);

		/* set up the small icon's bitmap */
		srcBits.baseAddr = (Ptr) (*theSICN)[theIndex];
		srcBits.rowBytes = 2;
		SetRect(&srcBits.bounds, 0, 0, 16, 16);

		/* draw the small icon in the current grafport */
		CopyBits(&srcBits,&(*qd.thePort).portBits,
			&srcBits.bounds,theRect,srcMode,nil);

		/* restore the resource's locked/unlocked condition */
		HSetState((Handle) theSICN, state);
	}
}

void CtoP(char *cstr, unsigned char *pstr) {
	short len = strlen(cstr);
	strncpy(pstr+1, cstr, len);
	pstr[0] = len;
}

// copy a c string to new pascal string, allocating memory for it
StringPtr CtoPCopy(const char *cstr)
{
	short len = strlen(cstr);
	StringPtr pstr = NewPtr(len + 1L);
	if (!pstr) return NULL;
	BlockMove(cstr, pstr+1, len);
	pstr[0] = len;
	return pstr;
}

void ErrorAlert(char *text) {
	Str255 errStr;
	short len = strlen(text);
	strncpy(errStr+1, text, len);
	//BlockMove(text, errStr+1, len);
	errStr[0] = len;

	ParamText(errStr, "\p", "\p", "\p");
	StopAlert(129, NULL);
}

void alertf(char *fmt, ...) {
	va_list ap;
	Str255 str;
	size_t len;

	va_start(ap, fmt);
	len = vsnprintf(str+1, sizeof(str)-1, fmt, ap);
	va_end(ap);
	str[0] = len;

	ParamText(str, "\p", "\p", "\p");
	StopAlert(129, NULL);
}

// append text to a textedit field
void TEAppendText(const void *text, long len, TEHandle hTE)
{
	static const short tabSize = 4;
	char *cur, *end, *dest;
	long newLen;
	short i;
	char c;

	end = (char *)text + len;
	TEPtr te = *hTE;
	Handle hText = te->hText;
	newLen = te->teLength + len;

	// Adjust length
	for (cur = (char *)text; cur < end; cur++) {
		switch (*cur) {
			// Expand tabs
			case '\t':
				newLen += tabSize;
				break;
			// Collapse \r\n
			case '\n':
				if (cur[-1] == '\r') {
					newLen--;
				}
				break;
		}
	}

	// Expand block if needed
	if (InlineGetHandleSize(hText) < newLen) {
		SetHandleSize(hText, newLen);
	}

	// Append the text
	dest = *hText + te->teLength;
	for (cur = (char *)text; cur < end; cur++) {
		switch (c = *cur) {
			case '\t':
				for (i = 0; i < tabSize; i++)
					*dest++ = ' ';
				break;
			case '\n':
				// Replace \r\n and \n with \r
				if (cur[-1] != '\r') {
					*dest++ = '\r';
				}
				break;
			default:
				*dest++ = c;
		}
	}

	te->teLength = newLen;
	TECalText(hTE);
}
