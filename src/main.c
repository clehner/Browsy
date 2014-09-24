#include <MacWindows.h>
#include <Events.h>
#include <Dialogs.h>
#include <ToolUtils.h>
#include <Sound.h>
#include <Traps.h>
#include <AppleEvents.h>
#include "Browsy.h"
#include "window.h"
#include "menus.h"
#include "utils.h"

void Initialize();
void MainLoop();
void CheckEnvironment();
void DoIdle();
void Terminate();
void HandleEvent(EventRecord *event);
void HandleUpdateEvt(EventRecord *event);
void HandleKeyDown(EventRecord *event, WindowPtr topWin);
void HandleMouseDown(EventRecord *event, WindowPtr topWin);
void HandleActivate(EventRecord *event);
void InitAppleEvents();
//void HandleNullEvent(EventRecord *event);

QDGlobals qd;

void main() {
	Initialize();

	// todo: open specified files
	// else open empty window
	PageWindowNavigateHome(NewPageWindow());

	MainLoop();
	Terminate();
}

void Initialize() {
	InitGraf(&qd.thePort);
	InitFonts();
	InitWindows();
	InitMenus();
	TEInit();
	InitDialogs(nil);
	InitCursor();

	CheckEnvironment();
	SetupMenus();
	InitAppleEvents();
	InitPageWindows();
}

void CheckEnvironment() {
	SysEnvRec	sEnv;
	OSErr		oe;

	oe = SysEnvirons(1,&sEnv);

	Sys7 = sEnv.systemVersion >= 0x0700;
	HasColorQD = sEnv.hasColorQD;

	HasWNE = (NGetTrapAddress(_WaitNextEvent, ToolTrap) !=
			  NGetTrapAddress(_Unimplemented, ToolTrap));
}

void MainLoop() {
	EventRecord event;
	Boolean ok;
	while (true) {
		ok = WNE(everyEvent, &event, 10L, nil);
		if (ok) {
			HandleEvent(&event);
		} else {
			// idle
		}
		DoIdle();
	}
}

void DoIdle() {
	PageWindow *topPWin = GetPageWindow(FrontWindow());
	if (topPWin) {
		PageWindowIdle(topPWin);
	}
}

void Terminate() {
	ExitToShell();
}

void HandleEvent(EventRecord *event) {
	WindowPtr topWin = FrontWindow();
	switch (event->what) {
		case mouseDown:
			HandleMouseDown(event, topWin);
			break;
		case keyDown:
		case autoKey:
			HandleKeyDown(event, topWin);
			break;
		case updateEvt:
			HandleUpdateEvt(event);
			break;
		case diskEvt:
			//HandleDiskEvt(event);
			break;
		case activateEvt:
			HandleActivate(event);
			break;
		case mouseMovedMessage:
			//HandleMouseMoved(event);
			break;
		case mouseUp:
			//HandleMouseUp(event);
			break;
		case nullEvent:
			//HandleNullEvent(event);
			break;
	}
}

void HandleMouseDown(EventRecord *event, WindowPtr topWin) {
	WindowPtr win;
	PageWindow* pWin;
	short windowCode = FindWindow(event->where, &win);
	Rect oldPort;
	switch (windowCode) {
		case inMenuBar:
			//AdjustMenus();
			HandleMenu(MenuSelect(event->where));
			break;
		case inSysWindow:
			SystemClick(event, win);
			break;
		case inDrag:
			DragWindow(win, event->where, &qd.screenBits.bounds);
			break;
		case inZoomIn:
		case inZoomOut:
			if (TrackBox(win, event->where, windowCode)) {
				SetPort(win);
				oldPort = win->portRect;
				EraseRect(&win->portRect);
				ZoomWindow(win, windowCode, false);
				//AdjustScrollBars(win, true);
				//DrawPage(win);
				InvalRect(&win->portRect);
				pWin = GetPageWindow(win);
				if (pWin) {
					PageWindowResized(pWin, oldPort);
				}
			}
			break;
		case inGrow:
			{
				long size;
				Rect r = qd.screenBits.bounds;
				//GrafPtr oldPort = 0;
				//GetPort(&oldPort);

				r.top = 100;
				r.left = 150;
				r.bottom -= (GetMBarHeight()<<1);
				r.right -= 20;
				//SetRect(&r, MaxWindowWidth+SBarSize-1, 64,
					//MaxWindowWidth+SBarSize-1, gMaxHeight);

				if (win != topWin) {
					SelectWindow(win);
				}

				oldPort = win->portRect;
				size = GrowWindow(win, event->where, &r);
				SizeWindow(win, LoWord(size), HiWord(size), true);
				pWin = GetPageWindow(win);
				if (pWin) {
					PageWindowResized(pWin, oldPort);
				}
				//DrawPage(win);
				SetPort(win);
				InvalRect(&win->portRect);
				//SetPort(oldPort);
			}
			break;
		case inContent:
			if (win != topWin) {
				SelectWindow(win);
				// todo: draw
			} else {
				pWin = GetPageWindow(win);
				if (pWin) {
					PageWindowMouseDown(pWin, event->where, event->modifiers);
				}
			}
			break;
		case inGoAway:
			if (TrackGoAway(win, event->where)) {
				if (event->modifiers & optionKey) {
					CloseAll();
				} else {
					pWin = GetPageWindow(win);
					ClosePageWindow(pWin);
				}
			}
			break;
	}
}

void HandleKeyDown(EventRecord *event, WindowPtr topWin) {
	PageWindow *pWin;
	char theChar = (char)(event->message & charCodeMask);
	if (event->modifiers & cmdKey) {
		HandleMenu(MenuKey(theChar));
	} else {
		pWin = GetPageWindow(topWin);
		PageWindowKeyDown(pWin, theChar);
	}
}

void HandleUpdateEvt(EventRecord *event) {
	WindowPtr win = (WindowPtr)(event->message);
	PageWindow *pWin = GetPageWindow(win);
	BeginUpdate(win);
	if (pWin) {
		UpdatePageWindow(pWin);
	}
	EndUpdate(win);
}

void HandleActivate(EventRecord *event) {
	WindowPtr win = (WindowPtr) event->message;
	PageWindow *pWin = GetPageWindow(win);
	if (!pWin) return;
	if (event->modifiers & activeFlag) {
		PageWindowActivate(pWin);
	} else {
		PageWindowDeactivate(pWin);
	}
}

/*void HandleNullEvent(EventRecord *event) {
	WindowPtr win = (WindowPtr) event->message;
	PageWindow *pWin = GetPageWindow(win);
	if (!pWin) return;
	PageWindowNullEvent(pWin, event);
}*/

Boolean GotRequiredParams(AppleEvent *event) {
   DescType returnedType;
   Size 	actualSize;
   OSErr	err;
   err = AEGetAttributePtr (event, keyMissedKeywordAttr,
						typeWildCard, &returnedType, NULL, 0,
						&actualSize);

   return err == errAEDescNotFound;

}	/* CAppleEvent::GotRequiredParams */


void HandleOpenAE(AppleEvent *event, AppleEvent *reply, long refCon) {
#pragma unused(reply, refCon)
	Handle		docList = NULL;
	long		i, itemCount;
	FSRef		myFSRef;
	AEDescList	theList;
	OSErr		oe;
	//PageWindow	*pwin = GetPageWindow(FrontWindow());
	if ((oe = AEGetParamDesc( event, keyDirectObject, typeAEList, &theList)) != noErr) {
		//DebugStr("\pAEGetParamDesc");
		return;
	}


	/*if (!GotRequiredParams(event)) {
		//DebugStr("\pGotRequiredParams");
		return;
	}*/

	if ((oe = AECountItems( &theList, &itemCount)) != noErr) {
		//DebugStr("\pAECountItems");
		return;
	}

	for (i = 1; i <= itemCount; i++) {
		//oe = AEGetNthPtr( &theList, i, typeFSRef, &aeKeyword, &actualType,
						//(Ptr) &myFSRef, sizeof( FSRef), &actualSize);
		oe = AEGetNthPtr( &theList, i, typeFSRef, NULL, NULL, &myFSRef, sizeof(FSRef), NULL);

		if (oe == noErr) {
			NewPageWindow();
			//OpenEditWindow(&myFSS);
		}
	}
	AEDisposeDesc(&theList);
	// event was handled successfully
}

pascal OSErr HandleAppleEvent(AppleEvent *event,AppleEvent *reply, long refCon) {
#pragma unused(reply, refCon)
	DescType	actualType;
	Size		actualSize;
	DescType	eventClass, eventID;
	OSErr		oe;

	if ((oe = AEGetAttributePtr( (AppleEvent*) event, keyEventClassAttr,
			typeType, &actualType, (Ptr) &eventClass,
			sizeof(eventClass), &actualSize)) != noErr)
		return oe;


	if ((oe = AEGetAttributePtr(  (AppleEvent*) event, keyEventIDAttr,
			typeType, &actualType, (Ptr) &eventID,
			sizeof(eventID), &actualSize)) != noErr)
		return oe;

	if (eventClass == kCoreEventClass) {
		switch (eventID) {
		case kAEOpenApplication:
			if (GotRequiredParams(event)) {
			}
			break;

		case kAEOpenDocuments:
			//HandleOpenEvent(event);
			break;

		case kAEPrintDocuments:
			break;

		case kAEQuitApplication:
			if (GotRequiredParams(event)) {
				Terminate();
			}
			break;
		}
	}

	return noErr;
}

void InitAppleEvents() {
	if (Sys7) {
		AEInstallEventHandler(typeWildCard, typeWildCard,
			(AEEventHandlerUPP) NewAEEventHandlerProc((ProcPtr) HandleAppleEvent), 0, FALSE);
		AEInstallEventHandler(kCoreEventClass, kAEOpenDocuments,
			NewAEEventHandlerUPP(HandleOpenAE), 0, FALSE);
			//(EventHandlerProcPtr) AppleEventHandler, 0, FALSE);
	}
}
