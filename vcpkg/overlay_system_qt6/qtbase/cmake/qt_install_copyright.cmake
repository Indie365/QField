#Could probably be the beginning of a vcpkg_install_copyright?
function(qt_install_copyright SOURCE_PATH)
    #Find the relevant license file and install it
    if(EXISTS "${SOURCE_PATH}/LICENSE.LGPLv3")
        set(LICENSE_PATH "${SOURCE_PATH}/LICENSE.LGPLv3")
    elseif(EXISTS "${SOURCE_PATH}/LICENSE.LGPL3")
        set(LICENSE_PATH "${SOURCE_PATH}/LICENSE.LGPL3")
    elseif(EXISTS "${SOURCE_PATH}/LICENSE.GPLv3")
        set(LICENSE_PATH "${SOURCE_PATH}/LICENSE.GPLv3")
    elseif(EXISTS "${SOURCE_PATH}/LICENSE.GPL3")
        set(LICENSE_PATH "${SOURCE_PATH}/LICENSE.GPL3")
    elseif(EXISTS "${SOURCE_PATH}/LICENSE.GPL3-EXCEPT")
        set(LICENSE_PATH "${SOURCE_PATH}/LICENSE.GPL3-EXCEPT")
    elseif(EXISTS "${SOURCE_PATH}/LICENSE.FDL")
        set(LICENSE_PATH "${SOURCE_PATH}/LICENSE.FDL")
    endif()
    file(INSTALL ${LICENSE_PATH} DESTINATION ${CURRENT_PACKAGES_DIR}/share/${PORT} RENAME copyright)
endfunction()