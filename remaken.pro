TARGET = remaken
VERSION=1.9.0

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
    src/commands/RemoteCommand.h \
    src/commands/SearchCommand.h \
    src/managers/BundleManager.h \
    src/commands/BundleXpcfCommand.h \
    src/commands/CleanCommand.h \
    src/commands/ConfigureCommand.h \
    src/Dependency.h \
    src/managers/DependencyManager.h \
    src/CmdOptions.h \
    src/Constants.h \
    src/Cache.h \
    src/commands/AbstractCommand.h \
#    src/HttpAsyncDownloader.h \
    src/commands/ListCommand.h \
    src/managers/XpcfXmlManager.h \
    src/tools/BrewSystemTool.h \
    src/tools/ConanSystemTool.h \
    src/tools/GitTool.h \
    src/commands/InfoCommand.h \
    src/commands/InitCommand.h \
    src/commands/InstallCommand.h \
    src/commands/PackageCommand.h \
    src/tools/NativeSystemTools.h \
    src/tools/PkgConfigTool.h \
    src/utils/DepUtils.h \
    src/utils/OsUtils.h \
    src/utils/PathBuilder.h \
    src/commands/ProfileCommand.h \
    src/commands/RunCommand.h \
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
    src/retrievers/SystemFileRetriever.h \
    src/tools/SystemTools.h \
    src/commands/ParseCommand.h \
    src/commands/BundleCommand.h \
    src/tools/VCPKGSystemTool.h \
    src/tools/ZipTool.h \
    src/tinyxml2.h \
    src/tinyxmlhelper.h

SOURCES += \
    src/commands/RemoteCommand.cpp \
    src/commands/SearchCommand.cpp \
    src/managers/BundleManager.cpp \
    src/commands/BundleXpcfCommand.cpp \
    src/commands/CleanCommand.cpp \
#    src/HttpAsyncDownloader.cpp \
    src/commands/ConfigureCommand.cpp \
    src/commands/ListCommand.cpp \
    src/managers/XpcfXmlManager.cpp \
    src/tools/BrewSystemTool.cpp \
    src/tools/ConanSystemTool.cpp \
    src/tools/GitTool.cpp \
    src/commands/InfoCommand.cpp \
    src/commands/PackageCommand.cpp \
    src/commands/InitCommand.cpp \
    src/tools/NativeSystemTools.cpp \
    src/tools/PkgConfigTool.cpp \
    src/utils/DepUtils.cpp \
    src/utils/OsUtils.cpp \
    src/utils/PathBuilder.cpp \
    src/commands/ProfileCommand.cpp \
    src/commands/RunCommand.cpp \
    src/tools/VCPKGSystemTool.cpp \
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
    src/retrievers/SystemFileRetriever.cpp \
    src/tools/SystemTools.cpp \
    src/commands/ParseCommand.cpp \
    src/commands/BundleCommand.cpp \
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
    samples/packagedependencies-artifactory.txt \
    samples/packagedependencies-brew.txt \
    samples/packagedependencies-github.txt \
    samples/packagedependencies-mixed.txt \
    resources/install_remaken_3rdparties.nsh \
    resources/install_remaken_custom_pages.nsh \
    samples/packagedependencies-system-linux.txt
