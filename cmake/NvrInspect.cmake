# cmake/NvrInspect.cmake
# Debug-print a target's important build settings with one call.
#
# Usage:
#   include(cmake/NvrInspect.cmake)
#   nvr_dump_target(${PROJECT_NAME})
#   # or, if you also use NvrDefs.cmake buckets:
#   nvr_dump_target(${PROJECT_NAME} BUCKET ${PROJECT_NAME})
#
# Notes:
# - Shows raw values (may include generator expressions like $<...>).
# - Works for libs and executables.
# - Optional BUCKET prints the flattened defs collected via nvr_defs_* helpers.

function(_nvr_dump__fmt OUT_VAR)
  if(ARGC GREATER 1)
    set(_list "${ARGV1}")
    if(_list)
      string(JOIN " " _joined ${_list})
      set(${OUT_VAR} "${_joined}" PARENT_SCOPE)
    else()
      set(${OUT_VAR} "" PARENT_SCOPE)
    endif()
  else()
    set(${OUT_VAR} "" PARENT_SCOPE)
  endif()
endfunction()

function(_nvr_dump__print KEY VAL)
  if(NOT VAL)
    message(STATUS "[nvr_dump] ${KEY}: (empty)")
  else()
    message(STATUS "[nvr_dump] ${KEY}: ${VAL}")
  endif()
endfunction()

# nvr_dump_target(<target> [BUCKET <name>])
function(nvr_dump_target TGT)
  if(NOT TARGET ${TGT})
    message(FATAL_ERROR "nvr_dump_target: target '${TGT}' does not exist")
  endif()

  set(_bucket "")
  if(ARGC GREATER 1)
    cmake_parse_arguments(NDR " " "BUCKET" "" ${ARGN})
    if(NOT "${NDR_BUCKET}" STREQUAL "")
      set(_bucket "${NDR_BUCKET}")
    endif()
  endif()

  # Basic
  get_target_property(_type              ${TGT} TYPE)
  get_target_property(_out_name          ${TGT} OUTPUT_NAME)
  get_target_property(_link_lang         ${TGT} LINKER_LANGUAGE)
  get_target_property(_cxx_std           ${TGT} CXX_STANDARD)
  get_target_property(_c_std             ${TGT} C_STANDARD)
  get_target_property(_pic               ${TGT} POSITION_INDEPENDENT_CODE)

  # Compile defs / options / features
  get_target_property(_compile_defs      ${TGT} COMPILE_DEFINITIONS)
  get_target_property(_iface_defs        ${TGT} INTERFACE_COMPILE_DEFINITIONS)
  get_target_property(_compile_opts      ${TGT} COMPILE_OPTIONS)
  get_target_property(_iface_copts       ${TGT} INTERFACE_COMPILE_OPTIONS)
  get_target_property(_compile_feats     ${TGT} COMPILE_FEATURES)
  get_target_property(_iface_cfeats      ${TGT} INTERFACE_COMPILE_FEATURES)

  # Includes
  get_target_property(_incs              ${TGT} INCLUDE_DIRECTORIES)
  get_target_property(_iface_incs        ${TGT} INTERFACE_INCLUDE_DIRECTORIES)

  # Linking
  get_target_property(_link_libs         ${TGT} LINK_LIBRARIES)
  get_target_property(_iface_llibs       ${TGT} INTERFACE_LINK_LIBRARIES)
  get_target_property(_link_opts         ${TGT} LINK_OPTIONS)
  get_target_property(_iface_lopts       ${TGT} INTERFACE_LINK_OPTIONS)

  # Sources (can be long)
  get_target_property(_sources           ${TGT} SOURCES)

  # Flatten lists for pretty print
  _nvr_dump__fmt(_compile_defs_f  "${_compile_defs}")
  _nvr_dump__fmt(_iface_defs_f    "${_iface_defs}")
  _nvr_dump__fmt(_compile_opts_f  "${_compile_opts}")
  _nvr_dump__fmt(_iface_copts_f   "${_iface_copts}")
  _nvr_dump__fmt(_compile_feats_f "${_compile_feats}")
  _nvr_dump__fmt(_iface_cfeats_f  "${_iface_cfeats}")
  _nvr_dump__fmt(_incs_f          "${_incs}")
  _nvr_dump__fmt(_iface_incs_f    "${_iface_incs}")
  _nvr_dump__fmt(_link_libs_f     "${_link_libs}")
  _nvr_dump__fmt(_iface_llibs_f   "${_iface_llibs}")
  _nvr_dump__fmt(_link_opts_f     "${_link_opts}")
  _nvr_dump__fmt(_iface_lopts_f   "${_iface_lopts}")
  _nvr_dump__fmt(_sources_f       "${_sources}")

  message(STATUS "---------------- nvr_dump_target('${TGT}') ----------------")
  _nvr_dump__print("TYPE"                    "${_type}")
  _nvr_dump__print("OUTPUT_NAME"             "${_out_name}")
  _nvr_dump__print("LINKER_LANGUAGE"         "${_link_lang}")
  _nvr_dump__print("CXX_STANDARD"            "${_cxx_std}")
  _nvr_dump__print("C_STANDARD"              "${_c_std}")
  _nvr_dump__print("POSITION_INDEPENDENT"    "${_pic}")

  _nvr_dump__print("COMPILE_DEFINITIONS"           "${_compile_defs_f}")
  _nvr_dump__print("INTERFACE_COMPILE_DEFINITIONS" "${_iface_defs_f}")
  _nvr_dump__print("COMPILE_OPTIONS"               "${_compile_opts_f}")
  _nvr_dump__print("INTERFACE_COMPILE_OPTIONS"     "${_iface_copts_f}")
  _nvr_dump__print("COMPILE_FEATURES"              "${_compile_feats_f}")
  _nvr_dump__print("INTERFACE_COMPILE_FEATURES"    "${_iface_cfeats_f}")

  _nvr_dump__print("INCLUDE_DIRECTORIES"           "${_incs_f}")
  _nvr_dump__print("INTERFACE_INCLUDE_DIRECTORIES" "${_iface_incs_f}")

  _nvr_dump__print("LINK_LIBRARIES"                "${_link_libs_f}")
  _nvr_dump__print("INTERFACE_LINK_LIBRARIES"      "${_iface_llibs_f}")
  _nvr_dump__print("LINK_OPTIONS"                  "${_link_opts_f}")
  _nvr_dump__print("INTERFACE_LINK_OPTIONS"        "${_iface_lopts_f}")

#   _nvr_dump__print("SOURCES"                       "${_sources_f}")

  if(NOT "${_bucket}" STREQUAL "")
    # If NvrDefs.cmake is included, show the bucket’s flattened defs.
    if(COMMAND nvr_defs_get)
      nvr_defs_get("${_bucket}" _bucket_defs)
      _nvr_dump__fmt(_bucket_defs_f "${_bucket_defs}")
      _nvr_dump__print("NVR_DEFS[${_bucket}]" "${_bucket_defs_f}")
    else()
      _nvr_dump__print("NVR_DEFS[${_bucket}]" "(NvrDefs.cmake not included)")
    endif()
  endif()

  message(STATUS "-----------------------------------------------------------")
endfunction()
