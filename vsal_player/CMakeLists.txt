include_directories(../vsal/include)
set(SRC vsal_player.cpp)
if(MSVC)
	set(SRC ${SRC} vsal_player.rc)
endif()
add_executable(vsal_player ${SRC})
target_link_libraries(vsal_player vsal opencv_core opencv_highgui opencv_imgproc)
if(UNIX)
	target_link_libraries(vsal_player ${Boost_LIBRARIES})
endif(UNIX)
set_target_properties(vsal_player PROPERTIES DEBUG_POSTFIX _d)
install(TARGETS vsal_player EXPORT vsal-targets DESTINATION bin COMPONENT bin)
