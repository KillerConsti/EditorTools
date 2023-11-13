// Copyright (C) 2012-2017 Promotion Software GmbH


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace user
{
	namespace editor
	{


		//[-------------------------------------------------------]
		//[ Public methods                                        ]
		//[-------------------------------------------------------]
		inline Plugin::~Plugin()
		{
			// Nothing to do in here
		}


		//[-------------------------------------------------------]
		//[ Public virtual qsf::Plugin methods                    ]
		//[-------------------------------------------------------]
		inline const char* Plugin::getName() const
		{
			return "Asset Collector Tool plugin";	// Please replace with a better name
		}

		inline const char* Plugin::getVendor() const
		{
			return "Unknown :)";	// Replace with your own name as author
		}

		inline const char* Plugin::getDescription() const
		{
			return "Used for copy assets";	// Insert a plugin description here
		}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
	} // editor
} // user
