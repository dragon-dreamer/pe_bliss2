macro(set_output_dirs target_name)
	set_target_properties("${target_name}"
		PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
		LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
		ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
		PDB_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
		COMPILE_PDB_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
endmacro()

macro(set_msvc_runtime_library target_name)
	set_property(TARGET "${target_name}" PROPERTY
		MSVC_RUNTIME_LIBRARY "${PE_BLISS_MSVC_RUNTIME_LIBRARY}")
endmacro()
