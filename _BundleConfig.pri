# Author(s) : Stéphane Leduc

# Include product configuration definitions
include(_ProductConfig.pri)

macx {

}

win32 {
    # 'Windows nsis parameters'
    SETUP_PRODUCTNAME=$${PRODUCT_DESCRIPTION}
    SETUP_FILEPFX=Setup_
    SETUP_FILENAME=$${PRODUCT_NAME}_$${VERSION}
    SETUP_MANUFACTURER=$${PRODUCT_MANUFACTURER}
    SETUP_VERSION=$${VERSION}
    SETUP_GUID=$${PRODUCT_GUID}
    SETUP_ICO_FILE=$$_PRO_FILE_PWD_/resources/appicon.ico
    SETUP_NSIS_INFO = CUSTOMIZE_ONINIT CUSTOMIZE_UNONINIT CUSTOMIZE_DISPLAY_PAGE_COMPONENTS CUSTOMIZE_ADDTOPATH CUSTOMIZE_ADD_CUSTOM_PAGE
    SETUP_NSIS_CUSTOM_FILEPATH=$$_PRO_FILE_PWD_/resources/install_remaken_3rdparties.nsh
    SETUP_NSIS_CUSTOM_PAGE_DEFINITION_FILEPATH=$$_PRO_FILE_PWD_/resources/install_remaken_custom_pages.nsh
}
