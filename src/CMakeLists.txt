project(DecUbiSndGui)
	
# Add the source files
set(SOURCE_FILES 
	./Pch.cpp
	./Gui/App.cpp
	./Gui/MainDialog.cpp
	./Gui/AppListView.cpp
	./Gui/FilesListView.cpp
	./Gui/SegmentsListView.cpp
	./Gui/AddManuallyDialog.cpp
	./Gui/AppListView.cpp
	./Functionality/FilesList.cpp
	./Functionality/SegmentsList.cpp
	./Functionality/Segment.cpp
	)
	
# If this is Windows then also add the resource file
#if(WIN32)
#	set(SOURCE_FILES ${SOURCE_FILES} ./Window.rc)
#endif(WIN32)

# Set the output path
set(EXECUTABLE_OUTPUT_PATH
	${PROJECT_BINARY_DIR}/../Binaries
	CACHE PATH
	"Single directory for all executables."
	)
include_directories(${PROJECT_SOURCE_DIR})

# Find wxWidgets
set(wxWidgets_USE_LIBS base core)
find_package(wxWidgets REQUIRED)
include(${wxWidgets_USE_FILE})

# Add targets
add_executable(DecUbiSndGui WIN32 ${SOURCE_FILES})
 
# Include the libraries
target_link_libraries(DecUbiSndGui ${wxWidgets_LIBRARIES})
