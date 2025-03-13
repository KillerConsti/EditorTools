// Copyright (C) 2012-2017 Promotion Software GmbH


//[-------------------------------------------------------]
//[ Header guard                                          ]
//[-------------------------------------------------------]
#pragma once


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include <qsf/message/MessageProxy.h>
#include <qsf/job/JobProxy.h>
#include <QtCore\qobject.h>
#include <QtWidgets\qaction.h>

#include <QtCore\qcoreapplication.h>
//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace qsf
{
	namespace editor
	{
		class AssetEditHelper;
	}
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace user
{
	namespace editor
	{


		//[-------------------------------------------------------]
		//[ Classes                                               ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*   stores settings*/
		class Settingsmanager// : QObject
		{
			//Q_OBJECT

		//[-------------------------------------------------------]
		//[ Public methods                                        ]
		//[-------------------------------------------------------]
		public:
			/**
		
			/**
			*  @brief
			*    Destructor
			*/
			static Settingsmanager* instance;
			Settingsmanager();
			~Settingsmanager();

			static void init();
			float GetBrushSize();
			void SetBrushSize(float BrushSize);
			void SetBrushIntensity(float newintensity);
			float GetBrushIntensity();
			int GetBrushShape();
			void SetBrushShape(int NewShape);

			void SaveSettings();
			void LoadSettings();
		private:
		int Brushshape;
		float BrushSize;
		float BrushIntensity;

		};


	//[-------------------------------------------------------]
	//[ Namespace                                             ]
	//[-------------------------------------------------------]
	} // editor
} // user

