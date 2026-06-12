set(TARGET_NAME yssocket)
if(EMSCRIPTEN)
	# WebRTC-capable implementation (browser hosting + room-code join);
	# provided by ysflight-web.
	add_library(${TARGET_NAME} ${YSFLIGHT_WEB_PORT_DIR}/yssocket/yssocket_emscripten.cpp)
else()
	add_library(${TARGET_NAME} yssocket.cpp)
endif()
target_include_directories(${TARGET_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
