SET(EMBREE_DIR "" CACHE PATH "base RIVL directory")
SET(EMBREE_BUILD_DIR "" CACHE PATH "base RIVL directory")
INCLUDE_DIRECTORIES(${EMBREE_DIR} ${EMBREE_DIR}/embree
  ${EMBREE_DIR}/examples/common 
  ${EMBREE_DIR}/examples/renderer
  ${EMBREE_DIR}/examples/renderer/renderer
  ${EMBREE_DIR}/examples/tutorials
  )
#INCLUDE_DIRECTORIES(${RIVL_DIR}/../host)


#INCLUDE(${IVL_DIR}/runtime/cmake/ivl.cmake)
INCLUDE(${EMBREE_DIR}/cmake/build_icc.cmake)
#INCLUDE(${IVL_DIR}/runtime/cmake/ivl.cmake)

SET(EMBREE_SOURCES "")

SET(EMBREE_SOURCES ${EMBREE_SOURCES}
 EmbreeManager.cc
 EmbreeManager.h
 ERenderable.cc
 ERenderable.h
 EScene.cc
 EScene.h
  )

#FIND_LIBRARY(LIB_RIVL_HOST rivl_host ${RIVL_DIR}/bin)
SET(LIB_EMBREE_DEVICE LIB_EMBREE_DEVICE-NOTFOUND)
SET(LIB_EMBREE_EMBREE LIB_EMBREE_EMBREE-NOTFOUND)
SET(LIB_EMBREE_LOADERS LIB_EMBREE_LOADERS-NOTFOUND)
SET(LIB_EMBREE_SYS LIB_EMBREE_SYS-NOTFOUND)
SET(LIB_EMBREE_LEXERS LIB_EMBREE_LEXERS-NOTFOUND)
# SET(LIB_EMBREE_DEVICE_SINGLERAY LIB_EMBREE_DEVICE_SINGLERAY-NOTFOUND)
FIND_LIBRARY(LIB_EMBREE_DEVICE device ${EMBREE_BUILD_DIR})
FIND_LIBRARY(LIB_EMBREE_EMBREE embree ${EMBREE_BUILD_DIR})
FIND_LIBRARY(LIB_EMBREE_LOADERS loaders ${EMBREE_BUILD_DIR})
FIND_LIBRARY(LIB_EMBREE_SYS sys ${EMBREE_BUILD_DIR})
FIND_LIBRARY(LIB_EMBREE_LEXERS lexers ${EMBREE_BUILD_DIR})
# FIND_LIBRARY(LIB_EMBREE_DEVICE_SINGLERAY device_singleray ${EMBREE_BUILD_DIR})


SET(EMBREE_LIBS 
  ${LIB_EMBREE_DEVICE}
  ${LIB_EMBREE_EMBREE}
  ${LIB_EMBREE_LOADERS}
  ${LIB_EMBREE_SYS}
  ${LIB_EMBREE_LEXERS}
  ${LIB_EMBREE_DEVICE_SINGLERAY}
  )

#IF (EMBREE_BUILD_DIR) 
  
  #ELSE (EMBREE_LIBS) 
    #MESSAGE(FATAL_ERROR "Manually Set Embree libraries")
    #ENDIF(EMBREE_LIBS)

ADD_LIBRARY (GLuRay_EMBREE STATIC ${EMBREE_SOURCES})

TARGET_LINK_LIBRARIES(GLuRay_EMBREE ${EMBREE_LIBS})


