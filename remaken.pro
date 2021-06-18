TARGET = remaken
VERSION=1.7.3

CONFIG += c++1z
CONFIG += console
CONFIG -= qt

DEFINES += MYVERSION=$${VERSION}
DEFINES += MYVERSIONSTRING=\\\"$${VERSION}\\\"

CONFIG(debug,debug|release) {
    DEFINES += _DEBUG=1
    DEFINES += DEBUG=1
}

CONFIG(release,debug|release) {
    DEFINES += NDEBUG=1
}

DEFINES += BOOST_ALL_NO_LIB

# Include bundle configuration parameters
include(_BundleConfig.pri)

win32:CONFIG -= static
win32:CONFIG += shared
QMAKE_TARGET.arch = x86_64 #must be defined prior to include
DEPENDENCIESCONFIG = staticlib
CONFIG += app_setup
#NOTE : CONFIG as staticlib or sharedlib,  DEPENDENCIESCONFIG as staticlib or sharedlib, QMAKE_TARGET.arch and PROJECTDEPLOYDIR MUST BE DEFINED BEFORE templatelibconfig.pri inclusion
include (builddefs/qmake/templateappconfig.pri)

HEADERS += \
    src/commands/BundleXpcfCommand.h \
    src/commands/CleanCommand.h \
    src/Dependency.h \
    src/managers/DependencyManager.h \
    src/CmdOptions.h \
    src/Constants.h \
    src/Cache.h \
    src/commands/AbstractCommand.h \
    src/commands/InfoCommand.h \
    src/commands/InitCommand.h \
    src/commands/InstallCommand.h \
    src/PathBuilder.h \
    src/commands/ProfileCommand.h \
    src/commands/VersionCommand.h \
    src/FileHandlerFactory.h \
    src/retrievers/CredentialsFileRetriever.h \
    src/retrievers/HttpFileRetriever.h \
    src/retrievers/FSFileRetriever.h \
    src/retrievers/IFileRetriever.h \
    src/retrievers/AbstractFileRetriever.h \
    src/AsioWrapper.h \
    src/HttpHandlerFactory.h \
    src/retrievers/ConanFileRetriever.h \
    src/retrievers/VCPKGFileRetriever.h \
    src/retrievers/SystemFileRetriever.h \
    src/tools/SystemTools.h \
    src/commands/ParseCommand.h \
    src/commands/BundleCommand.h \
    src/tools/ZipTool.h \
    src/utils/OsTools.h \
    src/tinyxml2.h \
    src/tinyxmlhelper.h

SOURCES += \
    src/commands/BundleXpcfCommand.cpp \
    src/commands/CleanCommand.cpp \
    src/commands/InfoCommand.cpp \
    src/commands/InitCommand.cpp \
    src/utils/PathBuilder.cpp \
    src/commands/ProfileCommand.cpp \
    src/tools/ZipTool.cpp \
    src/main.cpp \
    src/Dependency.cpp \
    src/managers/DependencyManager.cpp \
    src/CmdOptions.cpp \
    src/Cache.cpp \
    src/commands/InstallCommand.cpp \
    src/commands/VersionCommand.cpp \
    src/commands/AbstractCommand.cpp \
    src/FileHandlerFactory.cpp \
    src/retrievers/CredentialsFileRetriever.cpp \
    src/retrievers/FSFileRetriever.cpp \
    src/retrievers/HttpFileRetriever.cpp \
    src/retrievers/AbstractFileRetriever.cpp \
    src/HttpHandlerFactory.cpp \
    src/retrievers/ConanFileRetriever.cpp \
    src/retrievers/VCPKGFileRetriever.cpp \
    src/retrievers/SystemFileRetriever.cpp \
    src/tools/SystemTools.cpp \
    src/commands/ParseCommand.cpp \
    src/commands/BundleCommand.cpp \
    src/utils/OsTools.cpp \
    src/tinyxml2.cpp \
    src/tinyxmlhelper.cpp

INCLUDEPATH += src

unix {
   # QMAKE_CXXFLAGS += --coverage
   # QMAKE_LFLAGS += --coverage
}

linux {
    LIBS += -ldl -lpthread
    QMAKE_CXXFLAGS += -std=c++17 -D_GLIBCXX_USE_CXX11_ABI=1
    LIBS += -L/usr/local/lib -lZipper-static -lz
    INCLUDEPATH += /usr/local/include
}

macx {
    DEFINES += _MACOS_TARGET_
    QMAKE_MAC_SDK= macosx
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.15
    QMAKE_CXXFLAGS += -fasm-blocks -x objective-c++ -std=c++17
    #LIBS += -L/usr/lib -lz -lssl -lcrypto -L/usr/local/lib -lZipper-static
    #Zipper dependency : https://github.com/sebastiandev/zipper
    LIBS += -L/usr/local/lib -lboost_system -lstdc++
    QMAKE_LFLAGS += -mmacosx-version-min=10.15 -v -lstdc++
    INCLUDEPATH += /usr/local/include
}

win32 {
    # windows libs
    contains(QMAKE_TARGET.arch, x86_64) {
        LIBS += -L$$(VCINSTALLDIR)lib/amd64 -L$$(VCINSTALLDIR)atlmfc/lib/amd64 -L$$(WINDOWSSDKDIR)lib/winv6.3/um/x64
    }
    else {
        LIBS += -L$$(VCINSTALLDIR)lib -L$$(VCINSTALLDIR)atlmfc/lib -L$$(WINDOWSSDKDIR)lib/winv6.3/um/x86
    }
    LIBS += -lshell32 -lgdi32 -lComdlg32
    # openssl libs dependencies
    LIBS += -luser32 -ladvapi32 -lCrypt32
}

INCLUDEPATH += libs/nlohmann-json/single_include libs/CLI11/include

DISTFILES += \
    packagedependencies.txt \
    samples/packagedependencies-github.txt \
    samples/packagedependencies-mixed.txt \
    resources/install_remaken_3rdparties.nsh \
    resources/install_remaken_custom_pages.nsh
