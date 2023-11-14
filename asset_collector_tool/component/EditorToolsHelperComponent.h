#pragma once
#include <qsf/component/Component.h>
#include <qsf/base/PropertyHelper.h>
#include "qsf/reflection/type/CampQsfAssetProxy.h"
#include <qsf/job/JobProxy.h>
#include <qsf/base/StringHash.h>
#include <qsf\map\Entity.h>
#include <qsf/message/MessageProxy.h>

namespace user
{
namespace editor
{
	class EditorToolsHelperComponent : public qsf::Component
	{
	public:
		static const uint32 COMPONENT_ID;

		EditorToolsHelperComponent(qsf::Prototype* prototype);
		~EditorToolsHelperComponent();
		std::vector<uint64> mSeats;
		
		bool mMovable;
		void SetGlobalGlossiness(float NewGlossiness);
		float GetGlobalGlossiness();
	protected:
		virtual bool onStartup() override;
		virtual void onShutdown() override;
		//void updateJob(const qsf::JobArguments& jobArguments);
		//qsf::JobProxy mJobProxy;
		private:
		float mGlossiness;
		qsf::MessageProxy mStartupMessageProxy;
		void startup(const qsf::MessageParameters& parameters);
		QSF_CAMP_RTTI()
	};
}
}
QSF_CAMP_TYPE_NONCOPYABLE(user::editor::EditorToolsHelperComponent)