macro(add_icon SCALE EXTRA)
  set(NAME "icon-m-commandline")
  set(PROF   z${SCALE})
  separate_arguments(EXTRAS UNIX_COMMAND "${EXTRA}")
  if(${SCALE} MATCHES "large")
      string(REPLACE "-large" "" SHORTSCALE ${SCALE})
  else()
      set(SHORTSCALE  ${SCALE})
  endif()
  set(OUTPUT ${PROF}/${NAME}.png)
  list(APPEND ICONS ${OUTPUT})
  add_custom_command(
    OUTPUT          ${OUTPUT}
    COMMAND         sailfish_svg2png -z ${SHORTSCALE} ${EXTRAS} ${CMAKE_CURRENT_SOURCE_DIR} ${PROF}
    MAIN_DEPENDENCY ${NAME}.svg
    COMMAND_EXPAND_LISTS
    VERBATIM
  )
  install(
    FILES       "${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT}"
    DESTINATION share/themes/sailfish-default/meegotouch/${PROF}/icons
  )
endmacro()

add_icon(1.0  "" )
add_icon(1.25 "")
add_icon(1.49 "")
add_icon(2.0 "")
add_icon(2.5 "")

add_icon(1.5-large "-s 36 48 60 72 108 144 128")
add_icon(1.5 "-s 36 48 72 96 144 192 150")
add_icon(1.75 "-w 1080")

add_custom_target(theme-icons
  ALL
  DEPENDS ${ICONS}
)
