#pragma once
#include <qsf/component/Component.h>
#include <qsf/base/PropertyHelper.h>
#include "qsf/reflection/type/CampQsfAssetProxy.h"
#include <qsf/job/JobProxy.h>
#include <qsf/base/StringHash.h>
#include <qsf\map\Entity.h>
#include <qsf/message/MessageProxy.h>
#include <qsf\math\Color3.h>
#include <qsf/debug/DebugDrawProxy.h>
namespace kc
{
	class KCIndicatorComponent : public qsf::Component
	{
	public:
		static const uint32 COMPONENT_ID;

	public:
		/**
		*  @brief
		*    Constructor
		*they shall help us to debug the map by creating some nice indicators
		*  @we will create them ingame and read them via GameManager once we are back in editor
		*/
		KCIndicatorComponent(qsf::Prototype* prototype);

		/**
		*  @brief
		*    Destructor
		*/
		~KCIndicatorComponent();
		enum Color
		{
			WHITE = 0xffffff,
			RED = 0xffff00,
			YELLOW = 0xff0000,
			GREEN = 0x00ff00,
			BLUE = 0x0000ff
		};

		 Color getColor();
		void setColor(Color color);

		// Getter and setter for "IsAnimated" property
		bool isAnimated();
		void setIsAnimated(bool animated);


		//[-------------------------------------------------------]
		//[ Protected virtual qsf::Component methods              ]
		//[-------------------------------------------------------]
	protected:
		//[-------------------------------------------------------]
		//[ Lifecycle                                             ]
		//[-------------------------------------------------------]
		virtual bool onStartup() override;
		virtual void onShutdown() override;


		//[-------------------------------------------------------]
		//[ Private methods                                       ]
		//[-------------------------------------------------------]
	private:
		void updateJob(const qsf::JobArguments& jobArguments);
		void updateVisualization(bool onlyPosition = false);


		//[-------------------------------------------------------]
		//[ Private data                                          ]
		//[-------------------------------------------------------]
	private:
		Color				mColor;				///< Selected color of this indicator
		bool				mIsAnimated;		///< If "true", the indicator is animated
		qsf::Time			mAnimationTime;		///< Current time for animation

		qsf::JobProxy		mUpdateJobProxy;	///< Update job proxy, needed to get regular update calls
		qsf::DebugDrawProxy mDebugDrawProxy;	///< Debug draw proxy, needed for the visual representation
		QSF_CAMP_RTTI()
	};
}
QSF_CAMP_TYPE(kc::KCIndicatorComponent::Color)
QSF_CAMP_TYPE_NONCOPYABLE(kc::KCIndicatorComponent)