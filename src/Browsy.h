#ifndef _BROWSY_H
#define _BROWSY_H

//const short defaultALRT = 129;

#define BROWSY_VERSION "0.2.0"

Boolean HasColorQD;
Boolean Sys7;

typedef struct HistoryItem {
	char *title;
	char *address;
	struct HistoryItem *prev;
	struct HistoryItem *next;
} HistoryItem;

void Terminate();

#endif
