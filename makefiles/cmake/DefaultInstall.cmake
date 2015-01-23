cmake_policy( SET CMP0026 OLD )
if( NOT( CMAKE_MAJOR_VERSION LESS 3 ) )
set( CMAKE_PLATFORM_NO_SONAME_SUPPORT ON )
endif( NOT( CMAKE_MAJOR_VERSION LESS 3 ) )


if( WIN32 )
  if( __CLR__ )
    set( BINARY_OUTPUT_DIR . )
    set( SHARED_LIBRARY_OUTPUT_DIR . )
  else( __CLR__ )
    set( BINARY_OUTPUT_DIR bin )
    set( SHARED_LIBRARY_OUTPUT_DIR bin )
    set( SHARED_LIBRARY_LINK_DIR lib )
  endif( __CLR__ )
else( WIN32 )
   if( __LINUX64__ )
      set( BINARY_OUTPUT_DIR bin )
      set( SHARED_LIBRARY_OUTPUT_DIR lib64 )
      set( SHARED_LIBRARY_LINK_DIR lib64 )
   else( __LINUX64__ )
      set( BINARY_OUTPUT_DIR bin )
      set( SHARED_LIBRARY_OUTPUT_DIR lib )
      set( SHARED_LIBRARY_LINK_DIR lib )
   endif( __LINUX64__ )
endif( WIN32 )

if( __ANDROID__ )
else( __ANDROID__ )
    set( DEFAULT_WORKING_DIRECTORY ${BINARY_OUTPUT_DIR} )
endif( __ANDROID__ )



SET( HEADER_INSTALL_PREFIX include )
SET( DATA_INSTALL_PREFIX resources )

macro( install_default_dest )
   if( TARGET_BINARY_PATH )
	install( FILES ${OTHER}
		DESTINATION ${BINARY_OUTPUT_DIR}/${TARGET_BINARY_PATH}
		 )
	install( TARGETS ${ARGV}
		RUNTIME DESTINATION ${BINARY_OUTPUT_DIR}/${TARGET_BINARY_PATH}
		LIBRARY DESTINATION ${SHARED_LIBRARY_OUTPUT_DIR}/${TARGET_BINARY_PATH}
		ARCHIVE DESTINATION lib )
   else( TARGET_BINARY_PATH )
	install( FILES ${OTHER}
		DESTINATION ${BINARY_OUTPUT_DIR}
		 )
	install( TARGETS ${ARGV}
		RUNTIME DESTINATION ${BINARY_OUTPUT_DIR}
		LIBRARY DESTINATION ${SHARED_LIBRARY_OUTPUT_DIR}
		ARCHIVE DESTINATION lib )
   endif( TARGET_BINARY_PATH )
endmacro( install_default_dest )

macro( install_mode_dest )
    if( watcom21 )
        set( OTHER )
	foreach( lib ${ARGV} )
           get_property( filepath TARGET ${lib} PROPERTY LOCATION )
           string( REPLACE ".dll" ".sym" filepath ${filepath} )
           string( REPLACE ".exe" ".sym" filepath ${filepath} )
           #if( NOT ${filepath} MATCHES ".sym" )
           #   set( filepath ${filepath}.sym )
	   #endif()
           set( OTHER ${OTHER} ${filepath} )
        endforeach()
        #message( "OUT: ${OTHER}" )
    endif( watcom21 )
    if( __LINUX64__ )
	install( FILES ${OTHER}
		DESTINATION ${BINARY_OUTPUT_DIR}
		 )
	install( TARGETS ${ARGV}
		RUNTIME DESTINATION ${BINARY_OUTPUT_DIR}
		LIBRARY DESTINATION ${SHARED_LIBRARY_OUTPUT_DIR}
		ARCHIVE DESTINATION lib64 )
    else( __LINUX64__ )
	install( FILES ${OTHER}
		DESTINATION ${BINARY_OUTPUT_DIR}
		 )
	install( TARGETS ${ARGV}
		RUNTIME DESTINATION ${BINARY_OUTPUT_DIR}
		LIBRARY DESTINATION ${SHARED_LIBRARY_OUTPUT_DIR}
		ARCHIVE DESTINATION lib )
    endif( __LINUX64__ )
endmacro( install_mode_dest )

macro( install_sack_sdk_dest )
	install( TARGETS ${ARGV}
		RUNTIME DESTINATION ${SACK_BASE}/${BINARY_OUTPUT_DIR} 
		LIBRARY DESTINATION ${SACK_BASE}/${SHARED_LIBRARY_OUTPUT_DIR}
		ARCHIVE DESTINATION ${SACK_BASE}/lib )
endmacro( install_sack_sdk_dest )


macro( install_default_dest_binary )
if( TARGET_BINARY_PATH )
	install( TARGETS ${ARGV}
		RUNTIME DESTINATION ${BINARY_OUTPUT_DIR}/${TARGET_BINARY_PATH} 
		LIBRARY DESTINATION ${SHARED_LIBRARY_OUTPUT_DIR}/${TARGET_BINARY_PATH} )
else( TARGET_BINARY_PATH )
	install( TARGETS ${ARGV}
		RUNTIME DESTINATION ${BINARY_OUTPUT_DIR} 
		LIBRARY DESTINATION ${SHARED_LIBRARY_OUTPUT_DIR} )
endif( TARGET_BINARY_PATH )
endmacro( install_default_dest_binary )

if( OFF )
include( GetPrerequisites )

macro( install_product proj )
  get_prerequisites( ${${proj}_BINARY_DIR}/${proj} filelist 1 0 "" "" )
  #exclude_system recurse dirs
  message( "pre-req: [${filelist}] ${proj} (${${proj}_BINARY_DIR}/${PROJECT}${CMAKE_EXECUTABLE_SUFFIX})" )
endmacro( install_product )
endif( OFF )

macro( install_literal_product proj project_target )
if( __ANDROID__ )
  install( TARGETS ${proj} RUNTIME DESTINATION lib/${project_target} 
		LIBRARY DESTINATION lib/${project_target} 
	)
else( __ANDROID__ )
  if( WIN32 )
    if( watcom21 )
        set( OTHER )
	foreach( lib ${proj} )
           get_property( filepath TARGET ${lib} PROPERTY LOCATION )
           string( REPLACE ".dll" ".sym" filepath ${filepath} )
           string( REPLACE ".exe" ".sym" filepath ${filepath} )
           #if( NOT ${filepath} MATCHES ".sym" )
           #   set( filepath ${filepath}.sym )
	   #endif()
           set( OTHER ${OTHER} ${filepath} )
        endforeach()
        message( "OUT: ${OTHER}" )
	install( FILES ${OTHER}
		DESTINATION ${BINARY_OUTPUT_DIR}
		 )
    endif( watcom21 )
    if( __CLR__ )
      install( TARGETS ${proj} RUNTIME DESTINATION ./${project_target} 
	)
    else( __CLR__ )
      install( TARGETS ${proj} RUNTIME DESTINATION ${BINARY_OUTPUT_DIR}/${project_target}
			ARCHIVE DESTINATION lib		
	)
    endif( __CLR__ )
  else( WIN32 )
    install( TARGETS ${proj} 
	RUNTIME DESTINATION ${BINARY_OUTPUT_DIR}/${project_target} 
	LIBRARY DESTINATION ${BINARY_OUTPUT_DIR}/${project_target} 
        ARCHIVE DESTINATION lib
	)
endif()

endif( __ANDROID__ )
endmacro( install_literal_product )

macro( install_default_project proj project_target )

if( WIN32 )
  install( TARGETS ${proj} RUNTIME DESTINATION ${project_target} )
else( WIN32 )
  install( TARGETS ${proj} LIBRARY DESTINATION ${BINARY_OUTPUT_DIR}/${project_target} 
  		RUNTIME DESTINATION ${BINARY_OUTPUT_DIR}/${project_target} 
	)
endif()
endmacro( install_default_project )

macro( add_library_force_source project optional_style )
  if( ${optional_style} STREQUAL SHARED )
    if( FORCE_CXX )
      set_source_files_properties(${ARGN} PROPERTIES LANGUAGE CXX )
      set_source_files_properties( ${ARGN} PROPERTIES COMPILE_FLAGS /CLR )
    endif( FORCE_CXX )
  else( ${optional_style} STREQUAL SHARED )
    if( FORCE_CXX )
      set_source_files_properties( ${optional_style} ${ARGN} PROPERTIES LANGUAGE CXX )
      set_source_files_properties( ${optional_style} ${ARGN} PROPERTIES COMPILE_FLAGS /CLR )
    endif( FORCE_CXX )
  endif( ${optional_style} STREQUAL SHARED )

  add_library( ${project} ${optional_style} ${ARGN} )
  string( REPLACE "." "_" TARGET_LABEL ${project} )
  SET_PROPERTY(TARGET ${project} APPEND PROPERTY COMPILE_DEFINITIONS "TARGET_LABEL=${TARGET_LABEL};TARGETNAME=\"${CMAKE_LIBRARY_PREFIX}${project}${CMAKE_LIBRARY_SUFFIX}\"" )
endmacro( add_library_force_source )


macro( sack_add_library project optional_style )
  if( ${optional_style} STREQUAL SHARED )
    add_library( ${project} ${optional_style} ${FIRST_GCC_LIBRARY_SOURCE} ${ARGN} ${LAST_GCC_LIBRARY_SOURCE} )
  else( ${optional_style} STREQUAL SHARED )
    add_library( ${project} ${FIRST_GCC_LIBRARY_SOURCE} ${optional_style} ${ARGN} ${LAST_GCC_LIBRARY_SOURCE} )
  endif( ${optional_style} STREQUAL SHARED )
  string( REPLACE "." "_" TARGET_LABEL ${project} )
  SET_PROPERTY(TARGET ${project} APPEND PROPERTY COMPILE_DEFINITIONS "TARGET_LABEL=${TARGET_LABEL};TARGETNAME=\"${CMAKE_LIBRARY_PREFIX}${project}${CMAKE_LIBRARY_SUFFIX}\"" )
endmacro( sack_add_library )

macro( sack_add_literal_library project optional_style )
  if( ${optional_style} STREQUAL SHARED )
    add_library( ${project} ${optional_style} ${FIRST_GCC_LIBRARY_SOURCE} ${ARGN} ${LAST_GCC_LIBRARY_SOURCE} )
  else( ${optional_style} STREQUAL SHARED )
    add_library( ${project} ${FIRST_GCC_LIBRARY_SOURCE} ${optional_style} ${ARGN} ${LAST_GCC_LIBRARY_SOURCE} )
  endif( ${optional_style} STREQUAL SHARED )
  string( REPLACE "." "_" TARGET_LABEL ${project} )
  
  if( __ANDROID__ )        
	set( TARGETNAME "${target}" )
  else( __ANDROID__ )        
	set( TARGETNAME "${CMAKE_SHARED_LIBRARY_PREFIX}${target}${CMAKE_SHARED_LIBRARY_SUFFIX}" )
  endif( __ANDROID__ )        
  SET_PROPERTY(TARGET ${project} APPEND PROPERTY COMPILE_DEFINITIONS TARGET_LABEL=${TARGET_LABEL};TARETNAME=\"${TARGETNAME}\" )
  if( MOREDEFS )
     SET_TARGET_PROPERTIES(${project} PROPERTIES COMPILE_DEFINITIONS ${MOREDEFS} )
  endif( MOREDEFS )
  if( NOT __ANDROID__ )
		SET_TARGET_PROPERTIES(${project} PROPERTIES
                  SUFFIX ""
                  PREFIX ""
		)
  endif( NOT __ANDROID__ )
endmacro( sack_add_literal_library )


macro( add_executable_force_source project optional_style )
  if( optional_style STREQUAL WIN32 )
    if( FORCE_CXX )
      set_source_files_properties(${ARGN} PROPERTIES LANGUAGE CXX )
      set_source_files_properties( ${ARGN} PROPERTIES COMPILE_FLAGS /CLR )
    endif( FORCE_CXX )
  else( optional_style STREQUAL WIN32 )
    if( FORCE_CXX )
      set_source_files_properties( ${optional_style} ${ARGN} PROPERTIES LANGUAGE CXX )
      set_source_files_properties( ${optional_style} ${ARGN} PROPERTIES COMPILE_FLAGS /CLR )
    endif( FORCE_CXX )
  endif( optional_style STREQUAL WIN32 )
  add_executable( ${project} ${optional_style} ${ARGN} )
endmacro( add_executable_force_source )

macro( sack_add_executable project optional_style )
  if( __ANDROID__ )
      	add_program( ${project} ${optional_style} ${ARGN} )
  else( __ANDROID__ )
      
  	if( ${optional_style} STREQUAL WIN32 )
	  add_executable( ${project} ${optional_style} ${FIRST_GCC_PROGRAM_SOURCE} ${ARGN} ${LAST_GCC_PROGRAM_SOURCE} )
  	elseif( ${optional_style} STREQUAL DEPLOY )
	  add_executable( ${project} ${ARGN}  )
  	else( ${optional_style} STREQUAL WIN32 )
	  add_executable( ${project} ${FIRST_GCC_PROGRAM_SOURCE} ${optional_style} ${ARGN} ${LAST_GCC_PROGRAM_SOURCE} )
	endif( ${optional_style} STREQUAL WIN32 )
	  string( REPLACE "." "_" TARGET_LABEL ${project} )
	  SET_PROPERTY( TARGET ${project} APPEND PROPERTY COMPILE_DEFINITIONS "TARGETNAME=\"${project}${CMAKE_EXECUTABLE_SUFFIX}\";TARGET_LABEL=${TARGET_LABEL}" )
	  if( CMAKE_COMPILER_IS_GNUCC OR WATCOM )
	     SET_PROPERTY( TARGET ${project} APPEND PROPERTY COMPILE_DEFINITIONS "NO_DEADSTART_DLLMAIN" )
	  endif( CMAKE_COMPILER_IS_GNUCC OR WATCOM )
	  if( ${optional_style} STREQUAL WIN32 )
	  else( ${optional_style} STREQUAL WIN32 )
	     SET_PROPERTY( TARGET ${project} APPEND PROPERTY COMPILE_DEFINITIONS "CONSOLE_SHELL" )
	  endif( ${optional_style} STREQUAL WIN32 )
  endif( __ANDROID__ )
endmacro( sack_add_executable )


macro(my_target_link_libraries target )
    if(CMAKE_COMPILER_IS_GNUCC AND __ANDROID__ )
       foreach( target_lib ${ARGN} )
          if( ( TARGET ${target_lib} ) AND ( CMAKE_MAJOR_VERSION LESS 3 ) )
             get_property( lib_path TARGET ${target_lib} PROPERTY LOCATION)
             get_property( existing_outname TARGET ${target_lib} PROPERTY OUTPUT_NAME )
             if( NOT existing_outname )
		set( existing_outname ${target_lib} )
             endif( NOT existing_outname )
             if( ${lib_path} MATCHES "(.*)/([^/]*)$" )
                get_target_property(existing_link_flags ${target} LINK_FLAGS)
                if(existing_link_flags)
                    set(new_link_flags "${existing_link_flags} -L ${CMAKE_MATCH_1} -l ${existing_outname}")
                else()
                    set(new_link_flags "-L ${CMAKE_MATCH_1} -l ${existing_outname}")
                endif()
                set_target_properties( ${target} PROPERTIES LINK_FLAGS ${new_link_flags})
	         add_dependencies( ${target} ${target_lib} )
             endif( ${lib_path} MATCHES "(.*)/([^/]*)$" )
          else( ( TARGET ${target_lib} ) AND ( CMAKE_MAJOR_VERSION LESS 3 ) )
             target_link_libraries( ${target} ${target_lib} )
          endif( ( TARGET ${target_lib} ) AND ( CMAKE_MAJOR_VERSION LESS 3 ) )
       endforeach( target_lib ${ARGN} )
    else()
	target_link_libraries( ${target} ${ARGN} )
    endif()
endmacro()


macro( add_portable_program_ex portable targetname option1 )
  if( NOT SOURCES_ROOT )
    set( SACK_SOURCES_ROOT ${SACK_BASE}/src/sack )
  else( NOT SOURCES_ROOT )
    set( SACK_SOURCES_ROOT ${SOURCES_ROOT}/src/deadstart )
  endif( NOT SOURCES_ROOT )
	if( __ANDROID__ )
        	# CONSOLE_SHELL selects the entry of main or WinMain()
       		set( ExtraDefinitions "${ExtraDefinitions};CONSOLE_SHELL" )
		if( ${option1} STREQUAL WIN32 )
			if( portable )
				set( ExtraDefinitions "${ExtraDefinitions};BUILD_PORTABLE_EXECUTABLE" )
				add_library( ${targetname}.code SHARED ${ARGN} ${SOURCES_ROOT}/src/deadstart/deadstart_core.c )
			else( portable )
				add_library( ${targetname}.code SHARED ${ARGN} )
			endif( portable )
			my_target_link_libraries( ${targetname}.code ${SACK_LIBRARIES} ${SACK_PLATFORM_LIBRARIES} )
			set_target_properties( ${targetname}.code PROPERTIES COMPILE_DEFINITIONS "${ExtraDefinitions};ANDROID_SHELL;TARGETNAME=\"${targetname}.code\";TARGET_LABEL=${TARGET_LABEL}_code" )
			install_default_dest( ${targetname}.code )
                        if( EXISTS ${CMAKE_SOURCE_DIR}/android )
				add_library( ${targetname} SHARED android/android_native_app_glue android/default_android_main android/android_util )
                        else( EXISTS ${CMAKE_SOURCE_DIR}/android )
				add_library( ${targetname} SHARED ${SACK_SOURCES_ROOT}/android/android_native_app_glue ${SACK_SOURCES_ROOT}/android/default_android_main ${SACK_SOURCES_ROOT}/android/android_util )
                        endif( EXISTS ${CMAKE_SOURCE_DIR}/android )
			my_target_link_libraries( ${targetname} android log )
			string( REPLACE "." "_" TARGET_LABEL ${targetname} )
			set_target_properties( ${targetname} PROPERTIES COMPILE_DEFINITIONS "${ExtraDefinitions};ANDROID_SHELL;TARGETNAME=\"${targetname}\";TARGET_LABEL=${TARGET_LABEL}" )
			install_default_dest( ${targetname} )

			add_library( ${targetname}.client SHARED ${SACK_SOURCES_ROOT}/android/android_native_app_glue ${SACK_SOURCES_ROOT}/android/client_android_main ${SACK_SOURCES_ROOT}/android/android_client_util ${SACK_SOURCES_ROOT}/android/loader/android_elf )
			my_target_link_libraries( ${targetname}.client android log )
			string( REPLACE "." "_" TARGET_LABEL ${targetname}.client )
			set_target_properties( ${targetname}.client PROPERTIES COMPILE_DEFINITIONS "${ExtraDefinitions};ANDROID_SHELL;TARGETNAME=\"${targetname}.client\";TARGET_LABEL=${TARGET_LABEL}" )
			install_default_dest( ${targetname}.client )

		else( ${option1} STREQUAL WIN32 )
			if( portable )
				set( ExtraDefinitions "${ExtraDefinitions};BUILD_PORTABLE_EXECUTABLE" )
                               	add_executable( ${targetname} ${FIRST_GCC_PROGRAM_SOURCE} ${option1} ${ARGN} ${SOURCES_ROOT}/src/deadstart/deadstart_core.c ${LAST_GCC_PROGRAM_SOURCE} )
			else( portable )
                               	add_executable( ${targetname} ${FIRST_GCC_PROGRAM_SOURCE} ${option1} ${ARGN} ${LAST_GCC_PROGRAM_SOURCE}  )
			endif( portable )
			my_target_link_libraries( ${targetname} ${SACK_LIBRARIES} ${SACK_PLATFORM_LIBRARIES} )
			set_target_properties( ${targetname} PROPERTIES COMPILE_DEFINITIONS "${ExtraDefinitions};ANDROID_CONSOLE_UTIL;TARGETNAME=\"${targetname}\";TARGET_LABEL=${TARGET_LABEL}_exe" )
			install_default_dest( ${targetname} )
		endif( ${option1} STREQUAL WIN32 )
                
                
	else( __ANDROID__ )
		if( ${portable} )
			set( ExtraDefinitions "${ExtraDefinitions};BUILD_PORTABLE_EXECUTABLE" )
                        if( FORCE_CXX )
				if( NOT ${option1} STREQUAL WIN32 )
	                        	set_source_files_properties( ${option1} PROPERTIES LANGUAGE CXX )
	                        	set_source_files_properties( ${option1} APPEND PROPERTIES COMPILE_FLAGS /CLR )
				endif( NOT ${option1} STREQUAL WIN32 )
                        	set_source_files_properties(${SOURCES_ROOT}/src/deadstart/deadstart_core.c ${ARGN} PROPERTIES LANGUAGE CXX )
                               	set_source_files_properties(${SOURCES_ROOT}/src/deadstart/deadstart_core.c ${ARGN} PROPERTIES COMPILE_FLAGS /CLR )
                        endif( FORCE_CXX )
                        if( ${option1} STREQUAL WIN32 )
	                        if( NO_AUTO_DEADSTART_CORE )
					add_executable( ${targetname} ${option1} ${ARGN} )
                	        else( NO_AUTO_DEADSTART_CORE )
					add_executable( ${targetname} ${option1} ${FIRST_GCC_PROGRAM_SOURCE} ${SOURCES_ROOT}/src/deadstart/deadstart_core.c ${ARGN} ${LAST_GCC_PROGRAM_SOURCE} )
	                        endif( NO_AUTO_DEADSTART_CORE )
                        else( ${option1} STREQUAL WIN32 )
	                        if( NO_AUTO_DEADSTART_CORE )
					add_executable( ${targetname} ${option1} ${ARGN} )
                	        else( NO_AUTO_DEADSTART_CORE )
					add_executable( ${targetname} ${FIRST_GCC_PROGRAM_SOURCE} ${option1} ${SOURCES_ROOT}/src/deadstart/deadstart_core.c ${ARGN} ${LAST_GCC_PROGRAM_SOURCE} )
	                        endif( NO_AUTO_DEADSTART_CORE )
                        endif( ${option1} STREQUAL WIN32 )
		else( ${portable} )
                        if( FORCE_CXX )
				if( NOT ${option1} STREQUAL WIN32 )
	                        	set_source_files_properties( ${option1} PROPERTIES LANGUAGE CXX )
	                        	set_source_files_properties( ${option1} PROPERTIES COMPILE_FLAGS /CLR )
				endif( NOT ${option1} STREQUAL WIN32 )
                        	set_source_files_properties( ${ARGN} PROPERTIES LANGUAGE CXX )
                               	set_source_files_properties( ${ARGN} PROPERTIES COMPILE_FLAGS /CLR )
                        endif( FORCE_CXX )
                        if( ${option1} STREQUAL WIN32 )
				add_executable( ${targetname} ${option1} ${FIRST_GCC_PROGRAM_SOURCE} ${ARGN} ${LAST_GCC_PROGRAM_SOURCE} )
                        else( ${option1} STREQUAL WIN32 )
				add_executable( ${targetname} ${FIRST_GCC_PROGRAM_SOURCE} ${option1} ${ARGN} ${LAST_GCC_PROGRAM_SOURCE} )
                        endif( ${option1} STREQUAL WIN32 )
			my_target_link_libraries( ${targetname} ${SACK_LIBRARIES} )
		endif( ${portable} )
		string( REPLACE "." "_" TARGET_LABEL ${targetname} )
		if( ${option1} STREQUAL WIN32 )
			set_target_properties( ${targetname} PROPERTIES COMPILE_DEFINITIONS "${ExtraDefinitions};WINDOWS_SHELL;TARGETNAME=\"${targetname}\";TARGET_LABEL=${TARGET_LABEL}" )
		else()
			set_target_properties( ${targetname} PROPERTIES COMPILE_DEFINITIONS "${ExtraDefinitions};CONSOLE_SHELL;TARGETNAME=\"${targetname}\";TARGET_LABEL=${TARGET_LABEL}" )
		endif()
		if( MAKING_SACK_CORE )
			install_mode_dest( ${targetname} )
		else( MAKING_SACK_CORE )
			install_default_dest( ${targetname} )
		endif( MAKING_SACK_CORE )
		my_target_link_libraries( ${targetname} ${SACK_PLATFORM_LIBRARIES} )
	endif( __ANDROID__ )

endmacro( add_portable_program_ex )

macro( add_portable_program targetname option1 )
	add_portable_program_ex( 1 ${targetname} ${option1} ${ARGN} )
endmacro( add_portable_program )

macro( add_program targetname option1 )
	if( ${option1} STREQUAL WIN32 )
		add_portable_program_ex( 0 ${targetname} ${option1} ${ARGN} )
	else( ${option1} STREQUAL WIN32 )
		add_portable_program_ex( 0 ${targetname} ${option1} ${ARGN} )
	endif( ${option1} STREQUAL WIN32 )
endmacro( add_program )

macro( DEFINE_DEFAULT variable default )
if( NOT DEFINED ${variable} )
   #message( "variable ${variable} not defined (command line)" )
   set( ${variable} $ENV{${variable}} )
   if( "${${variable}}" STREQUAL "" )
     set( ${variable} ${default} )
   endif( "${${variable}}" STREQUAL "" )
endif( NOT DEFINED ${variable} )
endmacro( DEFINE_DEFAULT variable )
