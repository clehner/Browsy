#include <Resources.h>
#include <AppleEvents.h>
#include <StandardFile.h>
#include <ToolUtils.h>
#include <Sound.h>
#include <Menus.h>
#include <Devices.h>
#include <Events.h>
#include <Scrap.h>
#include "Browsy.h"
#include "window.h"
#include "menus.h"

const short defaultMenubar = 128;
const short aboutDialog = 128;

enum {appleMenu=128, fileMenu, editMenu, navigateMenu, historyMenu};

enum {
	fileNewItem=1, fileOpenItem, fileOpenURLItem,
	fileCloseItem=5, fileSaveAsItem,
	filePageSetupItem=8, filePrintItem,
	fileQuitItem=11,

	editUndoItem=1,
	editCutItem=3, editCopyItem, editPasteItem, editClearItem,
	editSelectAllItem=8,

	navigateBackItem=1, navigateForwardItem, navigateHomeItem,
	navigateStopItem=5, navigateReloadItem
};

static void ShowAbout();

QDGlobals qd;

void SetupMenus() {
	MenuHandle menu;
	Handle menubar = GetNewMBar(defaultMenubar);
	SetMenuBar(menubar);
	menu = GetMenuHandle(appleMenu);
	AppendResMenu(menu, 'DRVR');
	DrawMenuBar();
}

void HandleMenu(long menuAction) {

	short menu = HiWord(menuAction);
	short item = LoWord(menuAction);
	MenuHandle mh = GetMenuHandle(menu);
	WindowPtr topWin = FrontWindow();
	PageWindow* pWin = GetPageWindow(topWin);
	WindowPeek peek;
	Str255 name;
	GrafPtr savePort = 0;

	if (menuAction <= 0) {
		return;
	}

	switch(menu) {
		case appleMenu:
			if (item == 1) {
				ShowAbout();
			} else {
				GetPort(&savePort);
				GetMenuItemText(mh, item, name);
				OpenDeskAcc(name);
				SetPort(savePort);
			}
			break;
		case fileMenu:
			switch(item) {
				case fileNewItem:
					pWin = NewPageWindow();
					break;
				case fileQuitItem:
					// todo: wait for mactcp and name resolver
					Terminate();
					break;
				case fileOpenItem:
					PageWindowOpenFile(pWin);
					break;
				case fileOpenURLItem:
					if (!pWin) {
						pWin = NewPageWindow();
					}
					PageWindowFocusTE(pWin, pWin->addressBarTE);
					TESetSelect(0, (*pWin->addressBarTE)->teLength,
						pWin->addressBarTE);
					break;
				case fileCloseItem:
					peek = (WindowPeek)topWin;
					if (peek->windowKind < 0) {
						CloseDeskAcc(peek->windowKind);
					} else if (peek->goAwayFlag) {
						if (pWin) {
							ClosePageWindow(pWin);
						}
					}
					break;
				case fileSaveAsItem:
					PageWindowSaveAs(pWin);
					break;
				case filePageSetupItem:
					SysBeep(5);
					break;
				case filePrintItem:
					SysBeep(5);
					break;
			}
			break;
		case editMenu: {
			TEHandle te;
			Boolean inPage;

			// give to desk accessories first
			if (SystemEdit(item-1)) {
				break;
			}
			if (!pWin) {
				break;
			}
			te = pWin->focusTE;
			inPage = te == pWin->contentTE;
			switch (item) {
				case editUndoItem:
					// todo
					break;
				case editCutItem:
					if (te && !inPage) {
						TECut(te);
						if (!inPage) {
							//ZeroScrap();
							//TEToScrap();
						}
					} else {
						SysBeep(5);
					}
					break;
				case editCopyItem:
					if (te) {
						TECopy(te);
						if (!inPage) {
							//ZeroScrap();
							//TEToScrap();
						}
					}
					break;
				case editPasteItem:
					if (te && !inPage) {
						//TEFromScrap();
						//TEPaste(te);
					} else {
						SysBeep(5);
					}
					break;
				case editClearItem:
					if (te && !inPage) {
						TEDelete(te);
					} else {
						SysBeep(5);
					}
					break;
				case editSelectAllItem:
					if (te) {
						TESetSelect(0, (*te)->teLength, te);
					}
					break;
			}
			break;
		}
		case navigateMenu:
			switch(item) {
				case navigateBackItem:
					//PageWindowNavigateBack(pWin);
					PageWindowNavigateHistory(pWin, -1);
					break;
				case navigateForwardItem:
					//PageWindowNavigateForward(pWin);
					PageWindowNavigateHistory(pWin, 1);
					break;
				case navigateHomeItem:
					PageWindowNavigateHome(pWin);
					break;
				case navigateStopItem:
					PageWindowStop(pWin);
					break;
				case navigateReloadItem:
					PageWindowNavigateHistory(pWin, 0);
					//PageWindowReload(pWin);
					break;
			}
			break;
	}
	HiliteMenu(0);
}

pascal Boolean aboutFilter(DialogPtr dialog, EventRecord *event,
		short *itemHit) {
	switch (event->what) {
	case keyDown: {
		char key = event->message & charCodeMask;
		if (key == 'q' && event->modifiers & cmdKey) {
			Terminate();
		}
		// fall-through
	}
	case mouseDown:
		*itemHit = 1;
		return true;
	}

	return false;
}


static void ShowAbout() {
	DialogPtr dlg = GetNewDialog(aboutDialog, nil, (WindowPtr)-1L);
	short item;
	ModalDialog(aboutFilter, &item);
	DisposeDialog(dlg);
}
