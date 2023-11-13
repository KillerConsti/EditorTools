# This QMake project is only used to feed "lupdate.exe" which is used for internationalization
SOURCES      =	"asset_collector_tool/*.cpp" \
				"asset_collector_tool/plugin/*.cpp" \
				"asset_collector_tool/menu/*.cpp" \
				"asset_collector_tool/tool/*.cpp" \
				"asset_collector_tool/tool/*.ui" \
				"asset_collector_tool/view/*.cpp" \
				"asset_collector_tool/view/*.ui" \
				"asset_collector_tool/batchprocess/*.cpp" \
				"asset_collector_tool/map/test/*.cpp"
INCLUDEPATH +=	.
TRANSLATIONS =	"data/localization/de/asset_collector_tool.ts" \
				"data/localization/en/asset_collector_tool.ts"
