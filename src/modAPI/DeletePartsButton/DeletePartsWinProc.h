#pragma once

#include <Spore\BasicIncludes.h>

#define DeletePartsWinProcPtr intrusive_ptr<DeletePartsWinProc>

// To avoid repeating UTFWin:: all the time.
using namespace UTFWin;

class DeletePartsWinProc 
	: public IWinProc
	, public DefaultRefCounted
{
public:
	enum eDeletableParts {
		kDeleteAll = 0,
		kDeleteUnskinned = 1,
		kDeleteAccessories = 2,
		kDeleteNothing = -1
	};

	static const uint32_t TYPE = id("DeletePartsWinProc");
	
	DeletePartsWinProc();
	~DeletePartsWinProc();

	int AddRef() override;
	int Release() override;
	void* Cast(uint32_t type) const override;
	virtual eDeletableParts GetPriorityDeletableParts(const eastl::vector<EditorRigblockPtr>& blocks);
	
	int GetEventFlags() const override;
	// This is the function you have to implement, called when a window you added this winproc to received an event
	bool HandleUIMessage(IWindow* pWindow, const Message& message) override;
	
};
