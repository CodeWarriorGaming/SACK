macro( COPY_CPLUSPLUS FILE_LIST )
FOREACH(SOURCE ${ARGN} )
   if( ${SOURCE} MATCHES "${SACK_BASE}/(.*)\\.c$" )
      set( FILEOK 1 )
      set( BASENAME ${CMAKE_MATCH_1} )
      set( FILEEXT .cpp )
   elseif( ${SOURCE} MATCHES "${SACK_BASE}/(.*)\\.h$" )
      set( FILEOK 1 )
      set( BASENAME ${CMAKE_MATCH_1} )
      set( FILEEXT .h )
   elseif( ${SOURCE} MATCHES "(.*)\\.c$" )
      set( FILEOK 1 )
      set( BASENAME ${CMAKE_MATCH_1} )
      set( FILEEXT .cpp )
   elseif( ${SOURCE} MATCHES "(.*)\\.h$" )
      set( FILEOK 1 )
      set( BASENAME ${CMAKE_MATCH_1} )
      set( FILEEXT .h )
   else()
      set( FILEOK 0 )
      set( BASENAME "" )
   endif()
   
   if( FILEOK )
       get_source_file_property(SOURCE_FOLDER ${SOURCE} FOLDER)
       if( ${SOURCE} MATCHES "^${CMAKE_CURRENT_SOURCE_DIR}.*" )
          if( NOT ${SOURCE_FOLDER} MATCHES "NOTFOUND" )
            #message( "err folder : ${SOURCE_FOLDER} " )
            SOURCE_GROUP( ${SOURCE_FOLDER} FILES ${SOURCE} )
            SOURCE_GROUP( ${SOURCE_FOLDER} FILES ${CMAKE_BINARY_DIR}/${BASENAME}${FILEEXT} )
          endif()
          add_custom_command( OUTPUT ${CMAKE_BINARY_DIR}/${BASENAME}${FILEEXT}
                              DEPENDS ${SOURCE}
                              COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SOURCE} ${CMAKE_BINARY_DIR}/${BASENAME}${FILEEXT} 
                              COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_BINARY_DIR}/${BASENAME}${FILEEXT} 
                              )
       else( ${SOURCE} MATCHES "^${CMAKE_CURRENT_SOURCE_DIR}.*" )
         if( NOT ${SOURCE_FOLDER} MATCHES "NOTFOUND" )
           #message( "err folder : ${SOURCE_FOLDER} " )
           SOURCE_GROUP( ${SOURCE_FOLDER} FILES ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE} )
           SOURCE_GROUP( ${SOURCE_FOLDER} FILES ${CMAKE_BINARY_DIR}/${BASENAME}${FILEEXT} )
         endif()
         add_custom_command( OUTPUT ${CMAKE_BINARY_DIR}/${BASENAME}${FILEEXT}
                             DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE}
                             COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE} ${CMAKE_BINARY_DIR}/${BASENAME}${FILEEXT} 
                             COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_BINARY_DIR}/${BASENAME}${FILEEXT} 
                             )
       endif()
     set( ${FILE_LIST} ${${FILE_LIST}} ${CMAKE_BINARY_DIR}/${BASENAME}${FILEEXT} )
   endif()
ENDFOREACH(SOURCE)
endmacro( COPY_CPLUSPLUS )
