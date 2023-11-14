#include "asset_collector_tool/PrecompiledHeader.h"
#include <asset_collector_tool/component/EditorToolsHelperComponent.h>
#include <em5/EM5Helper.h>
#include <em5/game/Game.h>
#include <em5/gui/EmergencyGui.h>
#include <em5\map\EntityHelper.h>
#include <qsf\map\Map.h>
#include <qsf/renderer/compositor/DefaultCompositingComponent.h>
#include <em5/plugin/Jobs.h>
#include <qsf\map\Entity.h>
#include <qsf\QsfHelper.h>
#include <qsf\log\LogSystem.h>
#include <em5\plugin\Messages.h>
//[em5::PlacePersonFromBoatCommand]

namespace user
{
	namespace editor
	{
	const uint32 EditorToolsHelperComponent::COMPONENT_ID = qsf::StringHash("user::editor::EditorToolsHelperComponent");

	EditorToolsHelperComponent::EditorToolsHelperComponent(qsf::Prototype* prototype) : qsf::Component(prototype), mGlossiness(0.75f)
	{
	}

	EditorToolsHelperComponent::~EditorToolsHelperComponent()
	{
	}

	void EditorToolsHelperComponent::SetGlobalGlossiness(float NewGlossiness)
	{
		mGlossiness = NewGlossiness;
		auto EC =QSF_MAINMAP.getCoreEntity().getComponent<qsf::compositing::DefaultCompositingComponent>();
		if(EC == nullptr)
		{
			QSF_LOG_PRINTS(INFO, " user::editor::EditorToolsHelperComponent no DefaultCompositingComponent there")
			return;
		}
		EC->setGlobalGlossinessIntensity(mGlossiness);
		QSF_LOG_PRINTS(INFO," user::editor::EditorToolsHelperComponent set to "<< NewGlossiness << " by entity "<<getEntityId())
		
	}

	float EditorToolsHelperComponent::GetGlobalGlossiness()
	{
		return mGlossiness;
	}

	bool EditorToolsHelperComponent::onStartup()
	{
		mStartupMessageProxy.registerAt(qsf::MessageConfiguration(em5::Messages::GAME_STARTUP_FINISHED), boost::bind(&EditorToolsHelperComponent::startup, this, _1));
		return true;
	}

	void EditorToolsHelperComponent::onShutdown()
	{
	}

	void EditorToolsHelperComponent::startup(const qsf::MessageParameters & parameters)
	{
		SetGlobalGlossiness(GetGlobalGlossiness());
		mStartupMessageProxy.unregister();
	}

	}
}