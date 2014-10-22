#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <MacWindows.h>
#include <Memory.h>
#include <TextEdit.h>
#include <StandardFile.h>
#include <Menus.h>
#include <Sound.h>
#include <Fonts.h>
#include <Resources.h>
#include <ControlDefinitions.h>
#include <Controls.h>
#include "Browsy.h"
#include "utils.h"
#include "window.h"
#include "uri.h"
#include "document.h"

#define toolbarHeight 28
#define statusBarHeight 15
#define defaultWindow 128
#define handCursor 128
#define toolbarButtonsWidth 82
#define navBtn 1
#define toolbarIconsSICN 128
#define toolbarIconsDisabledSICN 129
#define navMenuPopupIds 1000
enum {iconBack, iconForward, iconHome, iconStop, iconRefresh};

/*static Rect toolbarRects[] = {toolbarRectBack, toolbarRectForward,
	toolbarRectHome, toolbarRectStop};*/

Rect toolbarButtonsRect = {4, 4, toolbarHeight, toolbarButtonsWidth + 4};

Rect toolbarRectBack		= {5,  5, 21, 21}; // top left bottom right
Rect toolbarRectForward		= {5, 25, 21, 41};
Rect toolbarRectHome		= {5, 45, 21, 61};
Rect toolbarRectStopRefresh	= {5, 65, 21, 81};

void Scroll(PageWindow *pWin, int h, int v);
//void PageWindowAdjustScrollBars(PageWindow *pWin);
void FrameAddressBar(PageWindow *pWin);
void PageWindowDrawGrowIcon(PageWindow *pWin);
HistoryItem *HistoryItemNewNext(HistoryItem *base);
void DrawToolbarButtons(PageWindow *pWin);
void HandleNavButtonClick(PageWindow *pWin, Point where);
void PopupNavMenu(PageWindow *pWin, Rect *buttonRect);
void DebugSave(long bytes, Ptr buffer);
void LoadingStarted(PageWindow *pWin);
void LoadingEnded(PageWindow *pWin);
void StopLoading(PageWindow *pWin);

pascal void ScrollAction(ControlHandle control, short part);

void PageURIOnStatus(void *obj, short httpStatus);
void PageURIOnHeader(void *obj, struct HTTPHeader *header);
void PageURIOnHeadersComplete(void *obj);
void PageURIOnMessageBegin(void *obj);
void PageURIOnData(void *obj, char *data, short len);
void PageURIOnClose(void *obj, short err);

URIConsumer PageURIConsumer = {
	.on_status = PageURIOnStatus,
	.on_header = PageURIOnHeader,
	.on_headers_complete = PageURIOnHeadersComplete,
	.on_message_begin = PageURIOnMessageBegin,
	.on_data = PageURIOnData,
	.on_close = PageURIOnClose
};

void InitPageWindows() {
	// left top right bottom
	//SetRect(&toolbarButtonsRect, 4, 4, toolbarButtonsWidth + 4, toolbarHeight);
	//(SICNHand) GetResource('SICN', mySICN);
}

PageWindow* GetPageWindow(WindowPtr win) {
	WindowPeek peek = (WindowPeek) win;
	if (!win) return nil;
	if (peek->windowKind >= userKind) {
		return (PageWindow*) GetWRefCon(win);
	}
	return nil;
}

void ClosePageWindow(PageWindow *pWin) {
	HistoryItem *history = pWin->history, *curr, *next;
	TEDispose(pWin->addressBarTE);
	TEDispose(pWin->contentTE);
	TEDispose(pWin->statusTE);
	DisposeWindow(pWin->window);

	// Dispose window history.
	if (history) {
		for (curr = history->next; curr; curr = next) {
			next = curr->next;
			if (curr->address) free(curr->address);
			if (curr->title) free(curr->title);
			free(curr);
		}
		for (curr = history; curr; curr = next) {
			next = curr->prev;
			if (curr->address) free(curr->address);
			if (curr->title) free(curr->title);
			free(curr);
		}
	}

	free(pWin);
}

void CloseAll() {
	WindowPeek win = (WindowPeek) FrontWindow();
	PageWindow* pWin;
	while (win) {
		pWin = GetPageWindow((WindowPtr)&win);
		if (pWin) {
			ClosePageWindow(pWin);
		}
		win = win->nextWindow;
	}
}

PageWindow* NewPageWindow() {
	//Str255 name = "\pUntitled";
	PageWindow* pWin = (PageWindow*) malloc(sizeof(PageWindow));
	WindowPtr window = GetNewWindow(defaultWindow, nil, (WindowPtr)-1L);
	TEHandle addressBarTE, contentTE, statusTE;
	TEPtr te;
	Rect destRect, viewRect, scrollRect, pr;
	//ControlHandle vScrollBar;

	SetPort(window);
	pr = window->portRect;

	//PageWindowDrawGrowIcon(pWin);

	// Address bar
	destRect.top = viewRect.top = 7;
	destRect.left = viewRect.left = 7 + toolbarButtonsWidth;
	destRect.bottom = viewRect.bottom = 21;
	destRect.right = 1000;
	viewRect.right = pr.right - pr.left - 7;
	addressBarTE = TENew(&destRect, &viewRect);
	//(*addressBarTE)->txFont = 0;
	//(*addressBarTE)->txFace = 0;
	(*addressBarTE)->txSize = 10;
	(*addressBarTE)->lineHeight = 13;
	(*addressBarTE)->fontAscent = 10;
	TESetText("http://", 7, addressBarTE);
	pWin->addressBarTE = addressBarTE;
	//TEActivate(addressBarTE);
	//InvalRect(&viewRect);

	// Content TE
	destRect.top = viewRect.top = pr.top + toolbarHeight;
	destRect.left = viewRect.left = 0;
	//destRect.bottom = viewRect.bottom = 30;
	viewRect.bottom = pr.bottom - statusBarHeight;
	destRect.bottom = 1000; // page length?
	destRect.right = viewRect.right = pr.right - 15;
	contentTE = TEStyleNew(&destRect, &viewRect);
	pWin->contentTE = contentTE;
	//TEActivate(contentTE);
	te = *contentTE;
	te->txFont = kFontIDMonaco;
	te->txFace = 0;
	te->txSize = 9;
	te->lineHeight = 12;
	te->fontAscent = 9;

	// Status bar TE
	//destRect.left = viewRect.left = 2;
	destRect.left = 2;
	viewRect.left = 0;
	//destRect.top = viewRect.top = pr.bottom - statusBarHeight + 4;
	destRect.top = pr.bottom - statusBarHeight + 2;
	viewRect.top = destRect.top - 1;
	//destRect.bottom = viewRect.bottom = destRect.top + 12;
	//destRect.bottom = viewRect.top + 12;
	//viewRect.bottom = pr.bottom;
	//statusTEPtr->destRect.right = pr.right - 16;
	//statusTEPtr->viewRect.right = pr.right;
	destRect.bottom = viewRect.top + 12;
	viewRect.bottom = pr.bottom;
	//viewRect.right = destRect.right = pr.right - 16;
	//destRect.right = pr.right - 16;
	//viewRect.right = pr.right;

	statusTE = TENew(&destRect, &viewRect);
	(*statusTE)->txSize = 9;
	(*statusTE)->lineHeight = 12;
	(*statusTE)->fontAscent = 9;
	(*statusTE)->crOnly = -1;
	//TESetText("Statusy", 7, statusTE);
	//InvalRect(&viewRect);
	pWin->statusTE = statusTE;

	//PageWindowSetStatus(pWin, "Statusy the status is good because it is a good status and here it is.");

	// Scrollbar
	scrollRect.left = pr.right - 15;
	scrollRect.top = pr.top - 1;
	scrollRect.right = pr.right + 1;
	scrollRect.bottom = viewRect.bottom;
	pWin->vScrollBar = NewControl(window, &scrollRect, "\p", true, 0, 0,
		scrollRect.bottom - scrollRect.top, 16, 1L);
	//pWin->vScrollBar = vScrollBar;
	//PageWindowAdjustScrollBars(pWin);

	/*pWin->toolbarBackBtn = NewControl(window, &toolbarRectBack, "\p", true,
		0, 0, 0, pushButProc, navBtn);*/

	SetWRefCon(window, (long) pWin);
	//SetWTitle(window, name);
	pWin->window = window;
	pWin->location = "";
	pWin->focusTE = NULL;
	pWin->history = NULL;
	pWin->isLoading = false;
	pWin->document = NULL;
	pWin->uri = NULL;

	FrameAddressBar(pWin);
	PageWindowAdjustScrollBars(pWin);
	DrawToolbarButtons(pWin);

	return pWin;
}

void DrawToolbarButtons(PageWindow *pWin) {
	SICNHand iconsActive = (SICNHand) GetResource('SICN', toolbarIconsSICN);
	SICNHand iconsDisabled = (SICNHand) GetResource('SICN', toolbarIconsDisabledSICN);
	HistoryItem *history = pWin->history;

	SetPort(pWin->window);
	//EraseRect(&toolbarButtonsRect);

	PlotSICN(&toolbarRectBack, history && history->prev ? iconsActive : iconsDisabled, iconBack);
	PlotSICN(&toolbarRectForward, history && history->next ? iconsActive : iconsDisabled, iconForward);
	PlotSICN(&toolbarRectHome, iconsActive, iconHome);
	PlotSICN(&toolbarRectStopRefresh,
			pWin->isLoading || history ? iconsActive : iconsDisabled,
			pWin->isLoading ? iconStop : iconRefresh);
}

void UpdatePageWindow(PageWindow *pWin) {
	WindowPtr win = pWin->window;
	Rect pr = win->portRect;
	Rect updateRect = (*win->visRgn)->rgnBBox;
	SetPort(win);
	//EraseRect(&updateRect);
	PageWindowDrawGrowIcon(pWin);

	if (RectInRgn(&(*(pWin->statusTE))->viewRect, win->visRgn)) {
		EraseRect(&(*(pWin->statusTE))->viewRect);
	}

	//(*(pWin->addressBarTE))->viewRect.right = pr.right - pr.left - 7;
	TEUpdate(&updateRect, pWin->addressBarTE);
	TEUpdate(&updateRect, pWin->contentTE);
	TEUpdate(&updateRect, pWin->statusTE);
	FrameAddressBar(pWin);

	UpdateControls(win, win->visRgn);

	if (RectInRgn(&toolbarButtonsRect, win->visRgn)) {
		DrawToolbarButtons(pWin);
	}

	//DrawDOMDocument(pWin->DOMDocument);
}

void PageWindowActivate(PageWindow *pWin) {
	WindowPtr win = pWin->window;
	ControlHandle ch;

	//SetPort(win);
	PageWindowDrawGrowIcon(pWin);
	if (pWin->focusTE) {
		TEActivate(pWin->focusTE);
	}
	//FrameAddressBar(pWin);
	//HiliteControl(pWin->vScrollBar, 0);

	for (ch = (ControlHandle) ((WindowPeek)(win))->controlList;
		ch != nil;
		ch = (*ch)->nextControl) {
			ShowControl(ch);
	}
}

void PageWindowDeactivate(PageWindow *pWin) {
	WindowPtr win = pWin->window;
	ControlHandle ch;

	//SetPort(win);
	//PageWindowDrawGrowIcon(pWin);
	if (pWin->focusTE) {
		TEDeactivate(pWin->focusTE);
	}
	//HiliteControl(pWin->vScrollBar, 255);

	//HideControl(pWin->vScrollBar);

	ch = (ControlHandle) ((WindowPeek)(win))->controlList;
	while (ch != nil) {
		HideControl(ch);
		ch = (*ch)->nextControl;
	}
	PageWindowDrawGrowIcon(pWin);
}

void PageWindowIdle(PageWindow *pWin) {
	WindowPtr win = pWin->window;
	//ControlHandle ch;
	Point mouse;
	Cursor *cursor;

	if (pWin->focusTE && (pWin->focusTE != pWin->contentTE)) {
		TEIdle(pWin->focusTE);
	}

	//SetPort(win);
	GetMouse(&mouse);
	SetPort(win);

	//if (FindControl(mouse, win, &ch))

	if (PtInRect(mouse, &(*(pWin->addressBarTE))->viewRect)) {
		cursor = *GetCursor(iBeamCursor);
	} else if (PtInRect(mouse, &(*(pWin->contentTE))->viewRect)) {
		cursor = *GetCursor(iBeamCursor);
	} else {
		//PageWindowKeyDown(pWin, 'a');
		// if (mouse is over <a>) cursor = handCursor;
		cursor = &qd.arrow;
	}
	SetCursor(cursor);
}

void PageWindowAdjustScrollBars(PageWindow *pWin) {
	short h;
	Rect pr = pWin->window->portRect;
	MoveControl(pWin->vScrollBar, pr.right - 15, -1 + toolbarHeight);
	SizeControl(pWin->vScrollBar, 16, (pr.bottom - pr.top) - 13 - toolbarHeight);
	h = TEGetHeight((*pWin->contentTE)->nLines, 1, pWin->contentTE);
	//SetCtlMax(pWin->vScrollBar, h);
	(*pWin->vScrollBar)->contrlMax = h;
}

void PageWindowResized(PageWindow *pWin, Rect oldPort) {
	Rect pr = pWin->window->portRect, r;
	TEPtr addressBarTEPtr = *(pWin->addressBarTE);
	TEPtr contentTEPtr = *(pWin->contentTE);
	TEPtr statusTEPtr = *(pWin->statusTE);
	addressBarTEPtr->viewRect.right = pr.right - pr.left - 7;
	contentTEPtr->viewRect.bottom = pr.bottom /*- pr.top*/ - statusBarHeight;
	contentTEPtr->viewRect.right = pr.right - 15;

	//statusTEPtr->viewRect.top = statusTEPtr->destRect.top = pr.bottom - statusBarHeight + 4;
	statusTEPtr->destRect.top = pr.bottom - statusBarHeight + 2;
	statusTEPtr->viewRect.top = statusTEPtr->destRect.top - 1;
	//statusTEPtr->viewRect.bottom = statusTEPtr->destRect.bottom = statusTEPtr->viewRect.top + 12;
	statusTEPtr->destRect.bottom = statusTEPtr->viewRect.top + 12;
	statusTEPtr->viewRect.bottom = pr.bottom;
	statusTEPtr->viewRect.right = statusTEPtr->destRect.right = pr.right - 15;

	InvalRect(&contentTEPtr->viewRect);
	EraseRect(&contentTEPtr->viewRect);

	EraseRect(&statusTEPtr->viewRect);
	InvalRect(&statusTEPtr->viewRect);

	PageWindowAdjustScrollBars(pWin);
	//PageWindowSetStatus(pWin, "asdfgasdfasdfy");

	r.left = (pr.right < oldPort.right ? pr.right : oldPort.right) - 8;
	r.right = pr.right;
	r.top = 0;
	r.bottom = toolbarHeight;
	EraseRect(&r);
	InvalRect(&r);

	//SetPort(pWin->window);
}

void PageWindowFocusTE(PageWindow *pWin, TEHandle te) {
	if (te != pWin->focusTE) {
		if (pWin->focusTE) {
			TEDeactivate(pWin->focusTE);
		}
		if (te) {
			TEActivate(te);
		}
		pWin->focusTE = te;
	}
}

void PageWindowMouseDown(PageWindow *pWin, Point where, int modifiers) {
	TEHandle te = NULL;
	ControlHandle ch;
	unsigned short part;
	int oldValue, delta;

	GlobalToLocal(&where);
	SetPort(pWin->window);

	part = (short)FindControl(where, pWin->window, &ch);
	if (!part) {
		if (PtInRect(where, &toolbarButtonsRect)) {
			HandleNavButtonClick(pWin, where);
		} else if (PtInRect(where, &((*(te = pWin->addressBarTE))->viewRect))) {
			// click in address bar
			//te = pWin->addressBarTE;
			PageWindowFocusTE(pWin, te);
			TEClick(where, (modifiers & shiftKey) != 0, te);
		} else if (PtInRect(where, &((*(te = pWin->contentTE))->viewRect))) {
			// click in page context text
			//te = pWin->contentTE;
			PageWindowFocusTE(pWin, te);
			TEClick(where, (modifiers & shiftKey) != 0, te);
			// hide insertion point
			if ((*te)->selStart == (*te)->selEnd) {
				TEDeactivate(te);
				pWin->focusTE = NULL;
			}
		}
	} else {
		switch(part) {
			// Form element
			case kControlButtonPart:
				//if (TrackControl(ch, where, 0)) {
				//if (where.top < toolbarHeight) {
				break;
			case kControlCheckBoxPart:
				break;

			// Scroll bar
			case kControlIndicatorPart:
				oldValue = GetControlValue(ch);
				if (TrackControl(ch, where, 0)) {
					delta = oldValue - GetControlValue(ch);
					if (delta) {
						if (ch == pWin->vScrollBar) {
							Scroll(pWin, 0, delta);
						} else {
							Scroll(pWin, delta, 0);
						}
					}
				}
				break;
			case kControlUpButtonPart:
			case kControlDownButtonPart:
			case kControlPageUpPart:
			case kControlPageDownPart:
				TrackControl(ch, where, ScrollAction);
				break;
		}
	}
}

pascal void ScrollAction(ControlHandle control, short part) {
	WindowPtr win = (*control)->contrlOwner;
	PageWindow *pWin = GetPageWindow(win);
	short delta, page, ex;
	Boolean isH;
	Rect r = (*control)->contrlRect;
	Rect wr = (*(pWin->contentTE))->viewRect; //pWin->window->portRect
	static unsigned long lastTicks;

	if (TickCount()-lastTicks < 4) return;
	else lastTicks = TickCount();
	isH = r.right-r.left > r.bottom-r.top;
	//page = (isH ? RoundDiv(r.right-r.left, win->hPitch)
		//: RoundDiv(r.bottom-r.top, win->vPitch))-1;
	page = isH ? wr.right-wr.left : wr.bottom-wr.top;
	switch(part) {
		case kControlUpButtonPart:		delta = 16;		break;
		case kControlDownButtonPart:	delta = -16;	break;
		case kControlPageUpPart:		delta = page;	break;
		case kControlPageDownPart:		delta = -page;	break;
		default:						delta = 0;
	}
	ex = GetControlValue(control) + delta;
	SetControlValue(control, ex);
	if (delta) {
		if (isH)
			Scroll(pWin, delta, 0);
		else
			Scroll(pWin, 0, delta);
		//PageWindowUpdate(pWin);
	}
}

void Scroll(PageWindow *pWin, int h, int v) {
	TEPinScroll(h, v, pWin->contentTE);
}

void HandleNavButtonClick(PageWindow *pWin, /*ControlHandle ch, */Point where) {
	Rect *r;// = (*ch)->contrlRect;
	Boolean hit = false;
	int i = 0;
	EventRecord evt;
	RgnHandle mouseRgn = NewRgn();
	Boolean btnHasMenu;

	/*short i;
	for (i=0; *i; i++) {
		if (PtInRect(where, toolbarRects[i])) {
			r = toolbarRects[i];
		}
	}*/
	if (PtInRect(where, &toolbarRectBack)) {
		if (!pWin->history->prev) return;
		r = &toolbarRectBack;
	} else if (PtInRect(where, &toolbarRectForward)) {
		if (!pWin->history->next) return;
		r = &toolbarRectForward;
	} else if (PtInRect(where, &toolbarRectHome)) {
		r = &toolbarRectHome;
	} else if (PtInRect(where, &toolbarRectStopRefresh)) {
		if (!pWin->isLoading && !pWin->history) return;
		r = &toolbarRectStopRefresh;
	}
	RectRgn(mouseRgn, r);
	//if (GetControlReference(ch) != navBtn) return;
	//if (!TrackControl(ch, where, nil)) return;
	//if (ch == pWin->toolbarBackBtn) {
	btnHasMenu = (r == &toolbarRectBack || r == &toolbarRectForward);
	do {
		GetMouse(&where);
		if (PtInRect(where, r)) {
			if (!hit) {
				hit = true;
				InvertRect(r);
			}
			if (btnHasMenu && (i > 8)) {
				PopupNavMenu(pWin, r);
				InvertRect(r);
				return;
			}
		} else {
			if (btnHasMenu) {
				PopupNavMenu(pWin, r);
				InvertRect(r);
				return;
			}
			if (hit) {
				hit = false;
				InvertRect(r);
			}
		}
		WNE(mUpMask | app4Mask, &evt, 10L, mouseRgn);
		i++;
	} while (Button());

	if (hit) {
		InvertRect(r);
		if (r == &toolbarRectBack) {
			PageWindowNavigateHistory(pWin, -1);
		} else if (r == &toolbarRectForward) {
			PageWindowNavigateHistory(pWin, 1);
		} else if (r == &toolbarRectHome) {
			PageWindowNavigateHome(pWin);
		} else if (r == &toolbarRectStopRefresh) {
			if (pWin->isLoading) {
				// stop
				StopLoading(pWin);
			} else {
				// refresh
				PageWindowNavigateHistory(pWin, 0);
			}
		} else {
			ErrorAlert("Unknown button pressed.");
		}
	}
}

/* I couldn't get AppendMenuItemText to work in 68k.
   This version takes a c string instead of pascal string.*/
void MyAppendMenuItemText(MenuHandle menuH, char *itemText) {
	/*Str255 ptext;
	CtoP(itemText, ptext);
	AppendMenu(menuH, ptext);*/
	static const char *metaChars = ";^!</(";
	Ptr menuData = (Ptr)(*menuH)->menuData;
	short menuTitleLength = menuData[0];
	Ptr menuItemData = menuData + menuTitleLength + 1;
	//Handle menuDataH = RecoverHandle(menuData);
	short menuItems = CountMItems(menuH)+1;
	short i = 0;
	Str255 sanitizedText;
	char replacedChars[255];
	short itemLength = strlen(itemText);
	memset(replacedChars, 0, 255);
	memcpy(sanitizedText+1, itemText, itemLength);
	sanitizedText[0] = itemLength;
	//CtoP(itemText, sanitizedText);
	// Strip out metacharacters
	while (itemText[i]
		&& ((i += strcspn(itemText+i, metaChars)+1))
		&& i<itemLength) {

		replacedChars[i] = sanitizedText[i];
		sanitizedText[i] = '*';
	}
	AppendMenu(menuH, sanitizedText);
	// Find the appended menu item data.
	for (i=1;i<menuItems;i++) {
		menuItemData += menuItemData[0] + 5;
	}
	// Put back metacharacters
	//for (i=0;i<1024 && menuItemData[i] != sanitizedText[0];i++) {}
	//menuItemData += i;
	if (memcmp(menuItemData, sanitizedText, itemLength) == 0) {
		for (i=0;i<itemLength;i++) {
			if (replacedChars[i]) menuItemData[i] = replacedChars[i];
		}
	}// else {
		//DebugSave(256, menuData);
	//}
	//Munger(menuDataH, menuTitleLength+1, sanitizedText+1, sanitizedText[0],
		//&itemText, sanitizedText[0]);
	//CalcMenuSize(menuH);
}

void PopupNavMenu(PageWindow *pWin, Rect *buttonRect) {
	char back = (buttonRect == &toolbarRectBack);
	MenuHandle menuH = NewMenu(navMenuPopupIds + back, "\pNav");
	HistoryItem *curr = pWin->history;
	//Str255 menuItemData;
	char *menuText;
	//short len;
	short choice;
	Point pt;

	while (curr = (back ? curr->prev : curr->next), curr) {
		menuText = curr->title ? curr->title : curr->address;
		MyAppendMenuItemText(menuH, menuText);
	}

	//PageWindowSaveAs(pWin, 256, (Ptr)(*menuH)->menuData);

	SetPt(&pt, buttonRect->left, buttonRect->bottom);
	LocalToGlobal(&pt);
	InsertMenu(menuH, -1);
	choice = PopUpMenuSelect(menuH, pt.v, pt.h, 0) & 0xffff;
	if (choice) {
		PageWindowNavigateHistory(pWin, back ? -choice : choice);
	}
	DeleteMenu(navMenuPopupIds + back);
	SetPort(pWin->window);
}

void StopLoading(PageWindow *pWin) {
	if (pWin->uri) {
		URIClose(pWin->uri);
		// free?
		pWin->uri = NULL;
		LoadingEnded(pWin);
	}
}

void PageWindowNavigate(PageWindow *pWin, char *location) {
	HistoryItem *historyItem;
	char *newLocation = url_sanitize(location);
	short len;
	if (!newLocation) {
		ErrorAlert("Unable to copy URI");
		return;
	}

	len = strlen(newLocation);

	TESetText(newLocation, len, pWin->addressBarTE);
	InvalRect(&(*pWin->addressBarTE)->viewRect);

	// don't put a new history item for the same address
	if (!pWin->history ||
			strncmp(newLocation, pWin->history->address, len+1) != 0) {
		historyItem = HistoryItemNewNext(pWin->history);
		if (historyItem) {
			historyItem->address = newLocation;
			//historyItem->title = "Title";
			pWin->history = historyItem;
			//PageWindowSetStatus(pWin, pWin->location);
		}
	}

	// redraw buttons
	InvalRect(&toolbarButtonsRect);

	// cancel previous request
	StopLoading(pWin);

	pWin->uri = NewURI(newLocation);
	if (!pWin->uri) {
		ErrorAlert("Unable to create URI request.");
		return;
	}
	URIConsume(pWin->uri, &PageURIConsumer, pWin);
	URIGet(pWin->uri);
}

/*
void RecievePageData(URIRequest* req) {
	PageWindow *pWin = (PageWindow *)req->refCon;
	URIResponse *resp = req->response;
	if (resp->offset == 0) {
		// started loading.
		pWin->location = pWin->history->address;
		LoadingStarted(pWin);
		if (pWin->document) {
			DisposeDOMDocument(pWin->document);
		}
		pWin->document = NewDOMDocument();
	}
	if (req->state == stateComplete) {
		// done loading
		LoadingEnded(pWin);
	}

	/*
	HLock(resp->contentHandle);
	DOMDocumentParseAppend(pWin->document, *resp->contentHandle + resp->offset,
		resp->length);
	HUnlock(resp->contentHandle);
	* /
	PageWindowAdjustScrollBars(pWin);

	TESetText(*(resp->contentHandle), resp->length, pWin->contentTE);

	InvalRect(&(*pWin->contentTE)->viewRect);
	EraseRect(&(*pWin->contentTE)->viewRect);
	//TEUpdate(&(*pWin->contentTE)->viewRect, pWin->contentTE);
}
*/

// delete items after given item, and replace with new one.
HistoryItem *HistoryItemNewNext(HistoryItem *base) {
	HistoryItem *curr, *next, *newNext;
	newNext = (HistoryItem *)malloc(sizeof(HistoryItem));
	if (!newNext) {
		ErrorAlert("Unable to create history item.");
		return NULL;
	}
	newNext->address = NULL;
	newNext->title = NULL;
	newNext->prev = base;
	newNext->next = NULL;
	if (base != NULL) {
		for (curr = base->next; curr; curr = next) {
			next = curr->next;
			if (curr->address) free(curr->address);
			if (curr->title) free(curr->title);
			free(curr);
		}
		base->next = newNext;
	}
	return newNext;
}

void PageWindowKeyDown(PageWindow *pWin, char theChar) {
	char *location;
	TEPtr te;
	if (!pWin->focusTE) return;
	te = *pWin->focusTE;

	if (pWin->focusTE == pWin->contentTE) {
	} else if (pWin->focusTE == pWin->addressBarTE) {
		switch(theChar) {
			case '\r':
			case '\n':
			case 3: // enter
				// go to the address
				if (te->teLength == 0 && pWin->history) {
					// Put back current address
					location = pWin->history->address;
					TESetText(location, strlen(location), pWin->addressBarTE);
					InvalRect(&te->viewRect);
				} else {
					size_t len = te->teLength;
					location = (char *) malloc((len + 1) * sizeof(char));
					strncpy(location, *te->hText, len);
					location[len] = '\0';

					PageWindowNavigate(pWin, location);
					free(location);
				}
				break;
			case 27: // delete
				// unprintable
				break;
			case 11: // page up
			case 1: // home 
				// up arrow
				TEKey(30, pWin->addressBarTE);
				break;
			case 12: // page down
			case 4: // end
				// down arrow
				TEKey(31, pWin->addressBarTE);
				break;
			case 127: // forward delete
				if (te->selStart != te->teLength) {
					TEKey(29, pWin->addressBarTE); // right
					TEKey(8, pWin->addressBarTE); // delete
				}
				break;
			default:
				// regular character
				TEKey(theChar, pWin->addressBarTE);
		}
	} else {
		if (theChar == '\r') {
			// todo: find and submit form
		} else {
			TEKey(theChar, pWin->focusTE);
		}
	}
}

void FrameAddressBar(PageWindow *pWin) {
	Rect frame = (*(pWin->addressBarTE))->viewRect;
	frame.left -= 3;
	frame.top -= 3;
	frame.right += 3;
	frame.bottom += 2;
	SetPort(pWin->window);
	FrameRect(&frame);
}

void PageWindowSetStatus(PageWindow *pWin, char *status) {
	//(*pWin->statusTE)->hText = &status;
	//TECalText(pWin->statusTE);
	TESetText(status, strlen(status), pWin->statusTE);
	InvalRect(&(*pWin->statusTE)->viewRect);
}

void PageWindowDrawGrowIcon(PageWindow *pWin) {
	Rect r = pWin->window->portRect;
	RgnHandle oldClip = pWin->window->clipRgn;
	SetPort(pWin->window);

	r.top = toolbarHeight - 1;

	pWin->window->clipRgn = NewRgn();
	ClipRect(&r);
	DrawGrowIcon(pWin->window);
	DisposeRgn(pWin->window->clipRgn);
	pWin->window->clipRgn = oldClip;

	// Draw toolbar line
	MoveTo(0, toolbarHeight - 1);
	Line(r.right - r.left, 0);
}

void PageWindowOpenFile(PageWindow *pWin) {
	short modifiers = 0; // todo
	SFTypeList tl;
	short numTypes;
	SFReply reply;
	char *pathName;
	char *encodedPathName;
	char uri[256] = "file:///";

	Point where;
	Rect *rp = &qd.screenBits.bounds;
	where.h = (rp->right - rp->left) - 348;
	where.v = (rp->bottom - rp->top) - 136;
	where.h >>= 1;
	where.v >>= 2;

	if (modifiers & optionKey)
		numTypes = -1;
	else {
		numTypes = 2;
		tl[0] = 'TEXT';
		tl[1] = 'HTML';
	}
	SFGetFile(where, "\pOpen...", NULL, numTypes, tl, nil, &reply);
	if (reply.good) {
		//char blah[255];
		short len;
		if (!pWin) {
			pWin = NewPageWindow();
		}

		pathName = GetFilePathName(reply.vRefNum, reply.fName);
		encodedPathName = url_encode(pathName);
		if (!encodedPathName) {
			alertf("Unable to convert file name");
			DisposePtr(pathName);
			return;
		}
		strncat(uri, encodedPathName, sizeof uri);

		free(encodedPathName);
		DisposePtr(pathName);

		PageWindowNavigate(pWin, uri);
	}
}

void PageWindowSaveAs(PageWindow *pWin) {
	short refNum;
	OSErr oe;
	SFReply reply;
	StringPtr fileName;
	Point where;
	Rect *rp = &qd.screenBits.bounds;
	//Str255 loc = "\pabcdefghijklmnopqrstuvwxyz";
	long bytes = 5;
	Ptr buffer;

	where.h = (rp->right - rp->left) - 348;
	where.v = (rp->bottom - rp->top) - 136;
	where.h >>= 1;
	where.v >>= 2;

	//GetFilename(pWin, fileName);
	/*
	ParamText("\pNot yet implemented.","\p","\p","\p");
	StopAlert(129, NULL);
	*/

	//BlockMove(pWin->location, loc+1, strlen(pWin->location));
	//loc[0] = strlen(pWin->location)+1;
	//ParamText("\pLocation:",loc,"\p","\p");
	//StopAlert(129, NULL);

	fileName = CtoPCopy(getFilePathFileName(pWin->location));
	//PageWindowSetStatus(pWin, fileName);
	SFPutFile(where, "\pSave As...", fileName, NULL, &reply);
	DisposePtr(fileName);
	if (!reply.good) {
		return;
	}

	//oe = SFPutOpen(reply.fName, reply.vRefNum, 'WWW6', 'TEXT', &refNum, NULL, NULL);
	oe = Create(reply.fName, reply.vRefNum, 'WWW6', 'TEXT');
	if (oe != noErr) {
		alertf("Unable to create file. %u", oe);
		return;
	}
	oe = FSOpen(reply.fName, reply.vRefNum, &refNum);
	if (oe != noErr) {
		ParamText("\pUnable to save file.","\p","\p","\p");
		StopAlert(129, NULL);
		//PageWindowSetStatus(pWin, errStr);
		return;
	}
	oe = SetFPos(refNum, fsFromStart, 0);
	if (oe != noErr) {
		ParamText("\pUnable to save file!","\p","\p","\p");
		StopAlert(129, NULL);
		return;
	}
	// todo
	buffer = "abcde";
	oe = FSWrite(refNum, &bytes, buffer);
	if (oe != noErr) {
		ParamText("\pUnable to write file.","\p","\p","\p");
		StopAlert(129, NULL);
		return;
	}
	FSClose(refNum);
}

void PageWindowNavigateHome(PageWindow *pWin) {
	//char *home = "http://192.168.1.128/stuff/election/2011candidates.html";
	//char *home = "file:///Macintosh HD/DOMDocuments/Browsy/page.html";
	//char *home = "about:Browsy";
	char *home = "about:stuff";
	//char *home = "file:///Macintosh HD/asdf.txt";
	//char *home = "file:///Untitled/Browsy";
	//char *home = "file:///Launcher/page.html";

	//"http://www.lehnerstudios.com/newsite/";
	// GetPrefStr(prefHomePage, home);
	PageWindowNavigate(pWin, home);
}

void PageWindowNavigateHistory(PageWindow *pWin, short amount) {
	HistoryItem *curr = pWin->history;
	short i = 1;
	// amount=0 for refresh
	if (curr == NULL) {
		i = 0;
	} else if (amount < 0) {
		// Back
		for (i=0; i>amount && curr->prev; i--) {
			curr = curr->prev;
		}
		pWin->history = curr;
	} else if (amount > 0) {
		// Forward
		for (i=0; i<amount && curr->next; i++) {
			curr = curr->next;
		}
		pWin->history = curr;
	}
	if (i == 0) {
		SysBeep(5);
	} else {
		PageWindowNavigate(pWin, curr->address);
	}
}

void DebugSave(long bytes, Ptr buffer) {
	short refNum;
	OSErr oe;
	SFReply reply;
	Point where;
	Rect *rp = &qd.screenBits.bounds;
	//Str255 loc = "\pabcdefghijklmnopqrstuvwxyz";

	where.h = (rp->right - rp->left) - 348;
	where.v = (rp->bottom - rp->top) - 136;
	where.h >>= 1;
	where.v >>= 2;

	ErrorAlert("Files not currently supported.");
	/*

	SFPutFile(where, "\pSave As...", "\pdebug", NULL, &reply);
	if (!reply.good) {
		return;
	}
	//oe = SFPutOpen(reply.fName, reply.vRefNum, 'WWW6', 'TEXT', &refNum, NULL, NULL);
	oe = Create(reply.fName, reply.vRefNum, 'WWW6', 'TEXT');
	/*if (oe != noErr) {
		ParamText("\pUnable to save file.","\p","\p","\p");
		StopAlert(129, NULL);
		return;
	}* /
	oe = FSOpen(reply.fName, reply.vRefNum, &refNum);
	if (oe != noErr) {
		ParamText("\pUnable to save file.","\p","\p","\p");
		StopAlert(129, NULL);
		return;
	}
	oe = SetFPos(refNum, fsFromStart, 0);
	if (oe != noErr) {
		ParamText("\pUnable to save file!","\p","\p","\p");
		StopAlert(129, NULL);
		return;
	}
	oe = FSWrite(refNum, &bytes, buffer);
	if (oe != noErr) {
		ParamText("\pUnable to write file.","\p","\p","\p");
		StopAlert(129, NULL);
		return;
	}
	FSClose(refNum);
	*/
}

void LoadingStarted(PageWindow *pWin) {
	pWin->isLoading = true;
	DrawToolbarButtons(pWin);
}

void LoadingEnded(PageWindow *pWin) {
	pWin->isLoading = false;
	DrawToolbarButtons(pWin);
}

// URI consumer callbacks

void PageURIOnStatus(void *obj, short httpStatus)
{
	PageWindow *pWin = (PageWindow *)obj;
	pWin->location = pWin->history->address;
	LoadingStarted(pWin);
	if (pWin->document) {
		DisposeDOMDocument(pWin->document);
	}
	pWin->document = NewDOMDocument();
	InvalRect(&(*pWin->contentTE)->viewRect);
	EraseRect(&(*pWin->contentTE)->viewRect);

	static char statusBuf[128];
	snprintf(statusBuf, sizeof(statusBuf), "status: %hu", httpStatus);
	PageWindowSetStatus(pWin, statusBuf);
}

void PageURIOnHeader(void *obj, HTTPHeader *header)
{
	PageWindow *pWin = (PageWindow *)obj;
	switch (header->name) {
		case httpContentType:
			//alertf("Got content type: %s", header->value);
			break;
		case httpContentLength:
			//alertf("Got content length: %s", header->value);
			break;
	}
}

void PageURIOnHeadersComplete(void *obj)
{
}

void PageURIOnMessageBegin(void *obj)
{
	PageWindow *pWin = (PageWindow *)obj;
	TESetText("", 0, pWin->contentTE);
}

void PageURIOnData(void *obj, char *data, short len)
{
	PageWindow *pWin = (PageWindow *)obj;
	//DOMDocumentParseAppend(pWin->document, data, len);

	//PageWindowAdjustScrollBars(pWin);

	TEAppendText(data, len, pWin->contentTE);

	InvalRect(&(*pWin->contentTE)->viewRect);
	EraseRect(&(*pWin->contentTE)->viewRect);
}

void PageURIOnClose(void *obj, short err)
{
	PageWindow *pWin = (PageWindow *)obj;
	LoadingEnded(pWin);
}
