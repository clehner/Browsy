#ifndef _UTILS_H
#define _UTILs_H

#include <Events.h>
#include <Quickdraw.h>
#include <TextEdit.h>

#define SIZE(array) (sizeof(array) / sizeof((array)[0]))
#define MIN(a,b) ((a) < (b) ? (a) : (b))

typedef short SICN[16];
typedef SICN **SICNHand;
void PlotSICN(Rect *theRect, SICNHand theSICN, long theIndex);

void CheckWNE();
Boolean WNE(EventMask eventMask, EventRecord *theEvent, UInt32 sleep, RgnHandle mouseRgn);
char *GetFilePathName(int vRefNum, Str255 fName);
int GetFilePathVolRef(char *pathFileName);
const char *getFilePathFileName(const char *pathFileName);
StringPtr CtoPCopy(const char *cstr);
char *url_encode(char *str);
char *url_decode(char *str);
char *url_sanitize(char *str);
//void PlotSICN(Rect *theRect, Handle theSICN, long theIndex);
void CtoP(char *cstr, unsigned char *pstr);
void ErrorAlert(char *text);
void alertf(char *fmt, ...);
void TEAppendText(const void *text, long length, TEHandle hTE);

#endif
