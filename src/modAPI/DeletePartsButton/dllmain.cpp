// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include "DeletePartsWinProc.h"

using namespace Editors;

UILayoutPtr deletePartsUI;
DeletePartsWinProcPtr deletePartsWinProc;

void Initialize()
{
	// This method is executed when the game starts, before the user interface is shown
	// Here you can do things such as:
	//  - Add new cheats
	//  - Add new simulator classes
	//  - Add new game modes
	//  - Add new space tools
	//  - Change materials
}

member_detour(EditorUI_Load, EditorUI, bool(cEditor*, uint32_t, uint32_t, bool))
{
	bool detoured(cEditor * pEditor, uint32_t instanceID, uint32_t groupID, bool editorModelForceSaveover)
	{
		bool res = original_function(this, pEditor, instanceID, groupID, editorModelForceSaveover);
		if (res && (Editor.mSaveExtension == TypeIDs::flr || Editor.mSaveExtension == TypeIDs::crt || Editor.mSaveExtension == TypeIDs::cll))
		{
			if (deletePartsUI == nullptr) deletePartsUI = new UTFWin::UILayout();
			deletePartsUI->LoadByID(id("DeletePartsUI"));
			deletePartsUI->SetParentWindow(Editor.mpEditorUI->mMainUI.FindWindowByID(0x0578EC50));	//History buttons

			if (deletePartsWinProc == nullptr) deletePartsWinProc = new DeletePartsWinProc();
			if (deletePartsUI->FindWindowByID(id("DeletePartsButton")) != nullptr)
				deletePartsUI->FindWindowByID(id("DeletePartsButton"))->AddWinProc(deletePartsWinProc.get());

			Math::Rectangle rect = deletePartsUI->FindWindowByID(id("DeletePartsButton"))->GetArea();
			rect.y1 -= 2.5f;
			rect.y2 -= 2.5f;
			deletePartsUI->FindWindowByID(id("DeletePartsButton"))->SetArea(rect);
		}
		return res;
	}
};

member_detour(Editor_Update, cEditor, void(float delta1, float delta2)) {
	void detoured(float delta1, float delta2)
	{
		original_function(this, delta1, delta2);
		IWindowPtr deleteButton = deletePartsUI->FindWindowByID(id("DeletePartsButton"));
		if (deleteButton != nullptr)
		{
			if (deletePartsWinProc->GetPriorityDeletableParts(Editor.mpEditorModel->mRigblocks) == DeletePartsWinProc::kDeleteNothing
				|| Editor.mMode != Mode::BuildMode)
				deleteButton->SetEnabled(false);
			else
				deleteButton->SetEnabled(true);
		}
	}
};

void Dispose()
{
	// This method is called when the game is closing
	deletePartsUI = nullptr;
	deletePartsWinProc = nullptr;
}

void AttachDetours()
{
	// Call the attach() method on any detours you want to add
	// For example: cViewer_SetRenderType_detour::attach(GetAddress(cViewer, SetRenderType));
	EditorUI_Load::attach(GetAddress(EditorUI, Load));
	Editor_Update::attach(GetAddress(cEditor, Update));
}


// Generally, you don't need to touch any code here
BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		ModAPI::AddPostInitFunction(Initialize);
		ModAPI::AddDisposeFunction(Dispose);

		PrepareDetours(hModule);
		AttachDetours();
		CommitDetours();
		break;

	case DLL_PROCESS_DETACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	}
	return TRUE;
}

