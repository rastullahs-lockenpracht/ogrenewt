ADD_EXECUTABLE(Demo07_CustomJoints demo07.cpp OgreNewtonApplication.cpp OgreNewtonFrameListener.cpp MyCustomBallSocket.cpp)

TARGET_LINK_LIBRARIES(Demo07_CustomJoints ${OGRE_LIBRARIES} ${CEGUI_LIBRARIES} ${OGRENEWT_LIBRARIES} ${OIS_LIBRARIES} ${CEGUIOGRE_LIBRARIES})

#INSTALL(TARGETS Demo07_CustomJoints RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/bin)
