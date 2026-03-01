#include "stdafx.h"
#include "DeletePartsWinProc.h"

using namespace Editors;

DeletePartsWinProc::DeletePartsWinProc()
{
}


DeletePartsWinProc::~DeletePartsWinProc()
{
}

// For internal use, do not modify.
int DeletePartsWinProc::AddRef()
{
	return DefaultRefCounted::AddRef();
}

// For internal use, do not modify.
int DeletePartsWinProc::Release()
{
	return DefaultRefCounted::Release();
}

// You can extend this function to return any other types your class implements.
void* DeletePartsWinProc::Cast(uint32_t type) const
{
	CLASS_CAST(Object);
	CLASS_CAST(IWinProc);
	CLASS_CAST(DeletePartsWinProc);
	return nullptr;
}

// This method returns a combinations of values in UTFWin::EventFlags.
// The combination determines what types of events (messages) this window procedure receives.
// By default, it receives mouse/keyboard input and advanced messages.
int DeletePartsWinProc::GetEventFlags() const
{
	return kEventFlagBasicInput | kEventFlagAdvanced;
}

bool DeleteParts(EditorRigblock* block, uint32_t excludeInstanceID, bool canBeDelete)
{
	if (block->mInstanceID != excludeInstanceID && canBeDelete)
	{
		Editor.mpEditorLimits->AddValue(0, block->mModelPrice);
		//delete the symmetric block with childrens to avoid using this function twice
		if (block->mpSymmetricRigblock != nullptr)
		{
			//SP::cSPEditorModel::DeleteBlock(SP::cSPEditorBlock* block, bool deleteChildren)
			CALL
			(
				Address(ModAPI::ChooseAddress(0x4acfd0, 0x4acfd0)),
				bool,
				Args(EditorModel*, EditorRigblock*, bool),
				Args(Editor.mpEditorModel, block->mpSymmetricRigblock.get(), true)
			);
		}
		//SP::cSPEditorModel::DeleteBlock(SP::cSPEditorBlock* block, bool deleteChildren)
		CALL
		(
			Address(ModAPI::ChooseAddress(0x4acfd0, 0x4acfd0)),
			bool,
			Args(EditorModel*, EditorRigblock*, bool),
			Args(Editor.mpEditorModel, block, false)
		);
		return true;
	}
	return false;
}

void ReplaceToNullGrasper(EditorRigblock* block)
{
	Editor.mpEditorLimits->AddValue(0, block->mModelPrice);
	if (block->mpSymmetricRigblock != nullptr)
	{
		block->mpSymmetricRigblock->mInstanceID = id("ce_grasper_null");
		block->mpSymmetricRigblock->mGroupID = 0x40626000;
	}
	block->mInstanceID = id("ce_grasper_null");
	block->mGroupID = 0x40626000;
}

DeletePartsWinProc::eDeletableParts DeletePartsWinProc::GetPriorityDeletableParts(const eastl::vector<EditorRigblockPtr>& blocks)
{
	if (!blocks.empty())
	{
		auto notVerbtebra = eastl::find_if(blocks.begin(), blocks.end(), [](const EditorRigblockPtr& block) { return block->mBooleanAttributes[kEditorRigblockModelIsVertebra] == false; });
		auto notPlantRoot = eastl::find_if(blocks.begin(), blocks.end(), [](const EditorRigblockPtr& block) { return block->mBooleanAttributes[kEditorRigblockModelIsPlantRoot] == false; });
		if (notVerbtebra != blocks.end() && notPlantRoot != blocks.end())
		{
			auto isTribalPart = eastl::find_if(blocks.begin(), blocks.end(), [](const EditorRigblockPtr& block) { return block->mModelRigBlockType.groupID == id("tribal"); });
			if (isTribalPart != blocks.end()) return kDeleteAccessories;

			auto isNotGrasperNull = eastl::find_if(blocks.begin(), blocks.end(), [](const EditorRigblockPtr& block) { return block->mLimbType == Editors::kLimbTypeNA && block->mInstanceID != id("ce_grasper_null") && block->mBooleanAttributes[kEditorRigblockModelUseSkin] == false; });
			if (isNotGrasperNull != blocks.end()) return kDeleteUnskinned;

			return kDeleteAll;
		}
	}
	return kDeleteNothing;
}

// The method that receives the message. The first thing you should do is probably
// checking what kind of message was sent...
bool DeletePartsWinProc::HandleUIMessage(IWindow* window, const Message& message)
{
	if (message.IsType(UTFWin::kMsgButtonClick) && message.source->GetControlID() == id("DeletePartsButton")) {
		Audio::PlayAudio(id("editor_general_click"));
		if (!Editor.mpEditorModel->mRigblocks.empty()) {
			bool forceUpdate = false;
			eDeletableParts deletable = GetPriorityDeletableParts(Editor.mpEditorModel->mRigblocks);
			for (int index = 0; index < Editor.mpEditorModel->mRigblocks.size(); index++) {
				EditorRigblockPtr block = Editor.mpEditorModel->mRigblocks[index];
				//check if block is not vertebra/plant root
				if (!block->mBooleanAttributes[Editors::kEditorRigblockModelIsVertebra]
					&& !block->mBooleanAttributes[Editors::kEditorRigblockModelIsPlantRoot])
				{
					//when the block was deleted, we should to reduce index so we can check every block on the creature
					switch (deletable)
					{
					case kDeleteAccessories:
						if (DeleteParts(block.get(), -1, block->mModelRigBlockType.groupID == id("tribal"))) index--;
						break;
					case kDeleteUnskinned:
						//ce_grasper_null is metaball that used on the end of limbs.
						//When player unsnap/delete a block, the metaball is always creating on it place
						//when we use SP::cSPEditorModel::DeleteBlock and doesn't happen, and so instead
						//we just replace the id of block to ce_grasper_null
						if (block->mBooleanAttributes[Editors::kEditorRigblockIsSnapped]
							&& block->mBooleanAttributes[Editors::kEditorRigblockModelHasBallConnector]
							&& !block->mBooleanAttributes[Editors::kEditorRigblockModelHasSocketConnector]
							&& block->mInstanceID != id("ce_grasper_null")) {
							ReplaceToNullGrasper(block.get());
							forceUpdate = true;
						}
						if (DeleteParts(block.get(), id("ce_grasper_null"), block->mLimbType == Editors::kLimbTypeNA)) index--;
						break;
					case kDeleteAll:
						if (DeleteParts(block.get(), -1, true)) index--;
						break;
					default:
						break;
					}
				}
			}
			Editor.CommitEditHistory(true);

			//we need update model after replacing blocks to metaballs
			if (forceUpdate) {
				//piece of code from Redo/Undo
				eastl::string16 tags = Editor.mpEditorNamePanel->mpLayout->FindWindowByID(0x5415e48)->GetCaption();
				EditorModel* newModel = new EditorModel();
				newModel->Load(Editor.mEditHistory[Editor.mEditHistory.size() - 1].get());
				Editor.SetEditorModel(newModel);
				Editor.SetName(newModel->mName.c_str());
				Editor.SetDescription(newModel->mDescription.c_str());
				Editor.SetTags(tags.c_str());
				newModel = nullptr;
			}
		}
	}
	// Return true if the message was handled, and therefore no other window procedure should receive it.
	return false;
}
