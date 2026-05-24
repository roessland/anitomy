if (EMSCRIPTEN)
	add_executable(anitomy-wasm
		wasm.cpp
	)

	set_target_properties(anitomy-wasm PROPERTIES
		OUTPUT_NAME anitomy
		SUFFIX ".wasm"
	)

	target_link_libraries(anitomy-wasm PRIVATE
		anitomy
		anitomy-config
	)

	target_compile_options(anitomy-wasm PRIVATE
		-Oz
		-flto
		-fno-rtti
		-fno-exceptions
		-fno-unwind-tables
		-fno-asynchronous-unwind-tables
	)

	target_link_options(anitomy-wasm PRIVATE
		-Oz
		-flto
		"-sSTANDALONE_WASM=0"
		"-sEXPORTED_FUNCTIONS=['_anitomy_parse','_anitomy_parse_elements','_anitomy_free','_malloc','_free']"
		"-sEXPORTED_RUNTIME_METHODS=[]"
		"-sALLOW_MEMORY_GROWTH=1"
		"-sINITIAL_MEMORY=4MB"
		"-sFILESYSTEM=0"
		"-sENVIRONMENT=web,worker,node,shell"
		"-sMALLOC=emmalloc"
		"-sSUPPORT_LONGJMP=0"
		"-sDISABLE_EXCEPTION_CATCHING=1"
		"--no-entry"
	)
else()
	add_executable(anitomy-cli
		main.cpp
	)

	set_target_properties(anitomy-cli PROPERTIES
		OUTPUT_NAME anitomy
	)

	target_link_libraries(anitomy-cli PRIVATE
		anitomy
		anitomy-config
	)
endif()
