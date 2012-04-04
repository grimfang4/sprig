set(sprig_SRCS
	SPG_misc.c
	SPG_primitives.c
	SPG_surface.c
	SPG_extended.c
	SPG_polygon.c
	SPG_rotation.c
)
set(sprig_HDRS
	sprig.h
	sprig_common.h
	sprig_inline.h
)
set(sprig_install_HDRS
	sprig.h
	sprig_inline.h
)

if(sprig_BUILD_SHARED)
	add_library(sprig_shared SHARED
		${sprig_HDRS}
		${sprig_SRCS}
	)
	set_target_properties(sprig_shared PROPERTIES
		OUTPUT_NAME "sprig"
		CLEAN_DIRECT_OUTPUT 1
		VERSION ${sprig_VERSION}
	)
	set_target_properties(sprig_shared PROPERTIES LINKER_LANGUAGE C)
endif()

if(sprig_BUILD_STATIC)
	add_library(sprig STATIC
		${sprig_HDRS}
		${sprig_SRCS}
	)
	set_target_properties(sprig PROPERTIES
		CLEAN_DIRECT_OUTPUT 1
		VERSION ${sprig_VERSION}
	)
	set_target_properties(sprig PROPERTIES LINKER_LANGUAGE C)
endif()

# These are used to create visual studio folders.
source_group(Common FILES ${sprig_SRCS} ${sprig_HDRS})

if(sprig_INSTALL)
	# install headers
	install(FILES ${sprig_install_HDRS} DESTINATION include/SDL)

	# install libraries
	if(sprig_BUILD_SHARED)
		install(TARGETS sprig_shared EXPORT sprig-targets DESTINATION lib)
	endif()
	if(sprig_BUILD_STATIC)
		install(TARGETS sprig EXPORT sprig-targets DESTINATION lib)
	endif()

	# install build system hooks for third-party apps
	#install(EXPORT sprig-targets DESTINATION ./sprig)
	#install(FILES sprigConfig.cmake DESTINATION ./sprig)
endif(sprig_INSTALL) 
