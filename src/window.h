#ifndef _WINDOW_H
#define _WINDOW_H

#include <MacWindows.h>
#include <TextEdit.h>
#include <Controls.h>
#include "document.h"
#include "Browsy.h"
#include "uri.h"

typedef struct {
	WindowPtr window;
	char *location;
	URI *uri; // resource request/response in progress
	TEHandle addressBarTE;
	TEHandle contentTE;
	TEHandle focusTE; // whichever TE has focus
	TEHandle statusTE; // text in status bar
	HistoryItem *history;
	ControlHandle vScrollBar;
	//ControlHandle toolbarBackBtn;
	Boolean isLoading;
	DOMDocument *document;
} PageWindow;

PageWindow* NewPageWindow();
PageWindow* GetPageWindow(WindowPtr win);
void InitPageWindows();
void ClosePageWindow(PageWindow *pWin);
void PageWindowNavigates(PageWindow *pWin, char *location);
void UpdatePageWindow(PageWindow *pWin);
void PageWindowActivate(PageWindow *pWin);
void PageWindowDeactivate(PageWindow *pWin);
void PageWindowAdjustScrollBars(PageWindow *pWin);
void PageWindowResized(PageWindow *pWin, Rect oldPort);
void CloseAll();
void PageWindowIdle(PageWindow *pWin);
void PageWindowMouseDown(PageWindow* pWin, Point where, int modifiers);
void PageWindowFocusTE(PageWindow *pWin, TEHandle te);
void PageWindowKeyDown(PageWindow *pWin, char theChar);
void PageWindowSetStatus(PageWindow *pWin, char *status);
void PageWindowSaveAs(PageWindow *pWin);
void PageWindowOpenFile(PageWindow *pWin);
void PageWindowNavigateHistory(PageWindow *pWin, short amount);
void PageWindowNavigateHome(PageWindow *pWin);

#endif
