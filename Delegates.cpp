#include "Delegates.h"

int DelegateHandle::CURRENT_ID = 0;

int DelegateHandle::GetNewID()
{
	int output = DelegateHandle::CURRENT_ID++;
	if (DelegateHandle::CURRENT_ID == 0)
		DelegateHandle::CURRENT_ID = 1;
	return output;
}
