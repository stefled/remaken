# Author(s) : Stéphane Leduc

# Include product configuration definitions
include(_ProductConfig.pri)

macx {

}

win32 {
    # 'Windows nsis parameters'
    SETUP_PRODUCTNAME=$${PRODUCT_DESCRIPTION}
    SETUP_FILEPFX=Setup_
    SETUP_FILENAME=$${PRODUCT_NAME}
    SETUP_MANUFACTURER=$${PRODUCT_MANUFACTURER}
    SETUP_VERSION=$${VERSION}
    SETUP_GUID=$${PRODUCT_GUID}
    SETUP_ICO_FILE=$$_PRO_FILE_PWD_/resources/appicon.ico
    SETUP_NSIS_INFO = CUSTOMIZE_ONINIT CUSTOMIZE_UNONINIT CUSTOMIZE_DISPLAY_PAGE_COMPONENTS CUSTOMIZE_ADDTOPATH CUSTOMIZE_ADD_CUSTOM_PAGE
    SETUP_NSIS_CUSTOM_FILEPATH=$$_PRO_FILE_PWD_/resources/install_remaken_3rdparties.nsh
    SETUP_NSIS_CUSTOM_PAGE_DEFINITION_FILEPATH=$$_PRO_FILE_PWD_/resources/install_remaken_custom_pages.nsh
    # override for copy qmake rules repository
    SETUP_COPYDIR=$$shell_quote($$shell_path($$OUT_PWD/setup))

    CLONEQMAKEREPO_CLEANCOMMAND = IF EXIST $${OUT_PWD}/setup $$QMAKE_DEL_TREE $$shell_quote($$shell_path($${OUT_PWD}/setup))
    remaken_clean_cloneqmakerepo.commands = $${CLONEQMAKEREPO_CLEANCOMMAND}

    QMAKE_REPO_URL = $$system(FOR /F $$shell_quote(tokens=*) %v IN (\'git --git-dir=$$_PRO_FILE_PWD_/builddefs/qmake/.git remote get-url origin\') do @echo %v)

    CLONEQMAKEREPO_COMMAND += git clone $$QMAKE_REPO_URL $$shell_quote($$shell_path($$OUT_PWD/setup/qmake))
    remaken_cloneqmakerepo.commands = $${CLONEQMAKEREPO_COMMAND}
    remaken_cloneqmakerepo.depends = remaken_clean_cloneqmakerepo

    create_setup.depends += remaken_cloneqmakerepo
    QMAKE_EXTRA_TARGETS  += remaken_cloneqmakerepo remaken_clean_cloneqmakerepo
}
