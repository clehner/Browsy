#ifndef _BROWSY_H
#define _BROWSY_H

//const short defaultALRT = 129;

Boolean HasColorQD;
Boolean Sys7;
Boolean HasWNE;

typedef struct HistoryItem {
	char *title;
	char *address;
	struct HistoryItem *prev;
	struct HistoryItem *next;
} HistoryItem;

#endif
