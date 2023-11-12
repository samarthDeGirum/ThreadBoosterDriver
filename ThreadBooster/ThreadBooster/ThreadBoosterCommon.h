#pragma once

/* 
* This structure defines a mutual way of sending data between User Mode and Kernel Mode 
*/

struct ThreadData {
	ULONG ThreadId; 
	int Priority;
};
