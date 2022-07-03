function (create_example example_name)
	set(PROJECT_NAME ${example_name})
	
	include_directories(../../AppCUI/include)
	
	add_executable(${PROJECT_NAME} main.cpp)
	add_dependencies(${PROJECT_NAME} AppCUI)
	
	target_link_libraries(${PROJECT_NAME} PRIVATE AppCUI)
	set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "Examples")
	
	include(CheckIPOSupported)
	check_ipo_supported(RESULT supported OUTPUT error)
	
	if( supported )
		message(STATUS "${PROJECT_NAME} => IPO / LTO enabled")
		set_property(TARGET ${PROJECT_NAME} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
	else()
		message(STATUS "${PROJECT_NAME} => IPO / LTO not supported: <${error}>")
	endif()
endfunction()

