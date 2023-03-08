# Remaken

**Remaken** is a meta dependencies management tool.

**Remaken** supports various C++ packaging systems using a standardized dependency file format (named **packagedependencies**), that follows a modular syntax.

**Remaken** also provide its own C++ packaging structure, based on [pkg-config](https://www.freedesktop.org/wiki/Software/pkg-config/) description files.

**Remaken** can be seen as the **"one tool to rule them all"** for C++ packagers.

## Overview

The need to use a tool such as **Remaken** comes from  :

- diversity of C++ package managers
- need to speed up development bootstrap
- need to shorten delays

Indeed, whatever the dependency your project needs, there is a chance the dependency already exists in binary format on one of the existing C++ packaging systems.

If it doesn't already exist in an existing C++ packaging systems, you can build your own binary version and share it as a **remaken dependency** across your team or organization, ensuring build options are the same.

It also avoids other developers to build locally the same dependency.

### Issues in native development without **remaken** and common development rules:

To setup a native C/C++ project that uses thirdparty dependencies, a developer must for each dependency (whatever the build system is in either [make](https://www.gnu.org/software/make/manual/make.html), [cmake](https://cmake.org/), [conan](https://conan.io/), [vcpkg](https://github.com/microsoft/vcpkg), [MSVC](https://visualstudio.microsoft.com/) …)  :

1. build each dependency with homogeneous build rules and flags (for instance c++ std version, for each target [*debug*, *release* | *os* | *cpu* | *shared*, *static*])
2. install each dependency in an accessible path in the system (and eventually, pollute the development environment system for instance using `make install`, or set a different *sysroot* - make install by default will need sudo access for unix(e)s.)
3. add include paths, libraries paths and libraries link flags for each dependency and sub dependency in its development project
4. For each new development, even based on same dependencies : reproduce every dependency build step 1 to 3 (also true for [conan](https://conan.io/) when the binary hosted version doesn’t fit with your options)
5. Running a final application : each dependency must either be copied in the same folder than the application or paths must be set to each shared dependency folder.
6. Last but not least, to package a final application with its dependencies means to bundle manually every shared dependency with the application before creating the final bundle/setup for the application<br>
NOTE: for most build systems ([make](https://www.gnu.org/software/make/manual/make.html), [cmake](https://cmake.org/), [Microsoft Visual Studio](https://visualstudio.microsoft.com/) builds ...) the target can be overwritten across versions. The solution would be to manually add custom target path to installation post-build steps - for each project.

Shared library or application project heterogeneity across a team can lead to integration issues as the build rules can slightly differ on each developer station.

### Development workflow using **remaken** / **builddefs-qmake** rules
**Remaken** :

- provide the same set of build rules across packaging systems (certified for packaging systems that perform a local build such as [conan](https://conan.io/), [vcpkg](https://github.com/microsoft/vcpkg), [brew](https://brew.sh/index_fr))
- install dependencies from any packaging system (including **remaken** own packaging and flag finding system) 
- there is no need to call manually each packaging system, to write a script … all is done at once from the **packagedependencies** description 
- build flags are populated either from remaken configure step or from [builddefs-qmake](https://github.com/b-com-software-basis/builddefs-qmake/releases/tag/builddefs-qmake-latest) rules (or other rules format will come in the future such as [cmake](https://cmake.org/), [bazel](https://bazel.build/) ...)
- run any application with appropriate environment deduced from described dependencies and/or from [xpcf](https://github.com/b-com-software-basis/xpcf) configuration file
- automate dependencies bundling within an application from dependencies description (either copying external dependencies in bundle destination folder or creating a shell script for native package managers such as apt, yum ...)
- bundle [xpcf](https://github.com/b-com-software-basis/xpcf) applications from [xpcf](https://github.com/b-com-software-basis/xpcf) configuration file
- integrate [conan](https://conan.io/) dependencies easily without writing a [conanfile.py](https://docs.conan.io/en/latest/reference/conanfile.html)
- provide a normalized package installation structure for **remaken** dependencies. cf [Package tree](#package-tree)
- allow to manage several dependencies version at the same time (each installation is based and searched with the package version number)
- binaries installation don't occur in system path. It avoids the pollution of the environment. It also avoids the need for sudo access.
- provide vcpkg installation and bootstrap
- provide [builddefs-qmake](https://github.com/b-com-software-basis/builddefs-qmake/releases/tag/builddefs-qmake-latest) rules installation and update
 
3/ with standardized build rules (currently qmake supported, future other system rules support can be implemented)
homogeneous builds ..
 
4/ Detailed design

## Installing **remaken**
### Linux Ubuntu 18.04 or mac OS X

First install [brew](https://brew.sh/index_fr) on your system:

```
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

Then enter the following commands from a terminal:

```
brew tap b-com/sft
brew install remaken
``` 

### Windows

Download and install the latest [setup file](https://github.com/b-com-software-basis/remaken/releases/) version.

## Package formats supported
### Cross platforms packaging systems :
- [conan](https://conan.io/)
- [vcpkg](https://github.com/microsoft/vcpkg)
- [brew](https://brew.sh/index_fr) (mac OS X and linux)
- **Remaken** packaging structure ([pkg-config](https://www.freedesktop.org/wiki/Software/pkg-config/) based)

### Dedicated system tools :
- apt-get (debian and derivatives)
- [pacman](https://wiki.archlinux.org/title/pacman) (archlinux)
- yum (redhat, fedora, centos)
- zypper (openSUSE)
- pkg (freebsd)
- pkgutil (solaris)
- [chocolatey](https://chocolatey.org/) (Windows)
- [scoop](https://scoop-docs.vercel.app/) (Windows)
- [others to come]

## Command line usage samples
### Initialize remaken default folder
- ```remaken init```: creates the default .remaken folder
- ```remaken init vcpkg```: initializes vcpkg (clone the vcpkg repository and bootstrap vcpkg)
- ```remaken init vcpkg [--tag tag]```: initializes vcpkg (clone the vcpkg repository for tag [tag] and bootstrap vcpkg)

It also initializes .remaken/rules/qmake with the builddefs-qmake release associated with the current remaken version. To get another version, use  ```remaken init --tag x.y.z```. 

You can also retrieve the latest qmake rules (latest rules reflects the current develop branch status) with ```remaken init --tag latest```.

All qmake rules are retrieved from [builddefs-qmake](https://github.com/b-com-software-basis/builddefs-qmake/releases/tag/builddefs-qmake-latest)

The ```remaken init``` command also supports the ```--force``` (alias ```-f```) to force reinstalling qmake rules and/or ```--override```  (alias ```-e```) to override existing files.

### Install IDE wizards
For now, only QtCreator wizards are provided.
To install the remaken, xpcf projects and classes wizards, use: 
- ```remaken init -w```
- ```remaken init wizards```

### Install Artefact Packager scripts

To install the artefact packager scripts for current OS in .remaken/scripts :
- ```remaken init artefactpkg```: initializes artefact packager script (download script on https://github.com/b-com-software-basis/remaken/releases/tag/artifactpackager)
- ```remaken init artefactpkg [--tag tag]```: initializes artefact packager script (download script on https://github.com/b-com-software-basis/remaken/releases/tag/artifactpackager for tag [tag])

### Set remaken packages root folder
The remaken packages folder is the location where packages are installed, searched from ...
This folder defaults to ```$(HOME)/.remaken/packages```.
It can be changed in various ways:
- define the ```REMAKEN_PKG_ROOT``` environment variable to another location
- create a ```.packagespath``` file in remaken root folder  ```$(HOME)/.remaken```. In this file, provide the path to your packages folder in the first line
- provide the package path with the ```--remaken-root``` parameter from remaken command line invocation
- put the ```remaken-root``` parameter and its value in the remaken ```config``` file in located in ```$(HOME)/.remaken```

### Managing remaken profiles

Remaken has profile support to set several options once for all for every projects (for instance, the os, the architecture, the C++ standard ...).

It also supports the creation of named profiles - it can be used for cross-platform builds (to target android systems for instance).

To initialize the default profile, use:
```remaken profile init```

You can also specify any value you want to set in the profile:
```remaken profile init --cpp-std 17```

To create a named profile, use:
```remaken profile init [profile_name or path]```

To use a named profile for any command, run:
```remaken --profile [profile_name or path] ...```

To display the default profile configuration:
```remaken profile display```

To display a named profile configuration:
```remaken --profile [profile_name or path] profile display```

To display options from the profile file and from default remaken values:
```remaken profile display -w```


### Searching dependencies
```remaken search [--restrict packaging_system_name] package_name [package_version] ```

```packaging_system_name``` is the name of the packaging system to search for this dependency. Its value depends on the operating system where remaken is used. 

On mac OS X and linux, its value is one of ```[brew, conan, system or vcpkg]```.

On windows, its value is one of ```[choco, conan, scoop, system or vcpkg]```.

### Managing remotes/repositories/ppa/taps/buckets ...

To get the list of installed remotes for every packaging system installed on the environment run:
```remaken remote list```

To get the list of remotes declared in a packagedependencies.txt, run (using ```--recurse``` will list recursively every remote declared in every packagedependencies file in the dependency tree):
```remaken remote listfile [--recurse] [path_to_remaken_dependencies_description_file.txt]```

To add the remotes declared in a packagedependencies.txt, run (using ```--recurse``` will add recursively every remote declared in every packagedependencies file in the dependency tree):
```remaken remote add [--recurse] [path_to_remaken_dependencies_description_file.txt]```

To get a sample how to declare an additional remote/bucket/ppa ... see the repository_url section in chapter [Dependency file syntax](#dependency-file-syntax)


### Installing/Configure dependencies
```remaken install [--conan_profile conan_profile_name] [-r  path_to_remaken_root] -i [-o linux] -t github [-l nexus -u http://url_to_root_nexus_repo] [--cpp-std 17] [-c debug|release] [--project_mode,-p] [path_to_remaken_dependencies_description_file.txt] [--condition name=value]* [--conan-build dependency]* ```

```remaken configure [--conan_profile conan_profile_name] [-r  path_to_remaken_root] -i [-o linux] -t github [-l nexus -u http://url_to_root_nexus_repo] [--cpp-std 17] [-c debug|release] [--project_mode,-p] [path_to_remaken_dependencies_description_file.txt] [--condition name=value]* ```

**Notes:**
 
- remaken_root defaults to ```$(HOME)/.remaken/packages``` or if ```REMAKEN_PKG_ROOT``` environment variable is defined, it defaults  to ```${REMAKEN_PKG_ROOT}```.
- ```remaken_dependencies_description_file``` defaults to current folder ```packagedependencies.txt```file
- ```[--project_mode,-p]```: enable project mode to generate project build files from packaging tools (conanbuildinfo.xxx, conanfile.txt ...).
   
   Project mode is enabled automatically when the folder containing the packagedependencies file also contains a QT project file
- ```[--conan_profile conan_profile_name] ``` allows to specify a specific conan profile

   When ```--conan_profile ``` is not specified:
  -  for native compilation: the ```default``` conan profile is used
  -  for cross compilation: remaken expects a ```[os]-[build-toolchain]-[architecture]``` profile to be available - for instance for android build, expects a ```android-clang-armv8``` profile

- ```[--conan_build dependency] ``` is a repeatable option, allows to specify to force rebuild of a conan dependency. Ex : ```--conan-build boost```.
- ```[--condition name=value] ``` is a repeatable option, allows to force a condition without application prompt (useful in CI). Ex : ```--condition USE_GRPC=true```. 

#### Configure Conditions

By default, ```remaken install``` and ```remaken configure``` prompt users for enabled or disabled configure conditions.

**Note:**
	A condition is defined just after library name.

a ```configure_conditions.pri``` file can be added into the project root folder to avoid user prompt :

	DEFINES+=CONDITION1
	DEFINES-=CONDITION2

of

After `remaken install` or `configure`, a `configure_conditions.pri` file is generated in ```.build-rules/[os]-[build-toolchain]-[architecture]``` folder with specified (by file or by user_prompt) enabled conditions.

The generated file is used when run remaken configure of qmake (on current project) for enable/disable dependencies automatically.
 
Then as only dependencies used are in the generated packagedependencies.txt, only enabled condtions are generated in this file.

#### Specific to Configure  - Currently not used in projects

Also generates build flags in remaken build rules folder (```.build-rules/[os]-[build-toolchain]-[architecture]```) :

- a master file ```dependenciesBuildInfo.pri``` includes
  - ```conanbuildinfo.pri``` : conan dependencies flags
  - ```remakenbuildinfo.pri``` : remaken dependencies flags 

This file can be used automatically in qmake projects by using ```use_remaken_parser``` value in ```DEPENDENCIESCONFIG```, ```CONFIG``` or ```REMAKENCONFIG``` value.
It disables package dependencies parsing made by **builddefs-qmake** rules.

###
 
### Bundling dependencies together
```remaken bundle [--recurse] [-d path_to_bundle_destination] [--cpp-std 11|14|17|20] [-c debug|release] [path_to_remaken_dependencies_description_file.txt]```

Note: ```remaken_dependencies_description_file``` defaults to current folder ```packagedependencies.txt```file.


### Bundling XPCF applications
```remaken bundleXpcf -d path_to_root_destination_folder -s relative_install_path_to_modules_folder --cpp-std 17 -c debug xpcfApplication.xml```

### Checking packagedependencies file format validity
```remaken parse [path_to_remaken_dependencies_description_file.txt]```

### Removing installed remaken dependencies
```remaken clean```

### Listing dependencies tree from a packagedependencies.txt file
```remaken info [path_to_remaken_dependencies_description_file.txt]```: displays the recursive dependency tree from the file.
```remaken info --pkg_systemfile -d path_to_write_pkgfile [path_to_remaken_dependencies_description_file.txt]```: generates pkg system files from packagedependencies files into specified folder - currently Only conan is managed.

### Listing remaken installed packages
The **list** command allows to :

- ```remaken list```: list all remaken installed packages
- ```remaken list [package name]```: list all installed versions of a remaken package 
- ```remaken list --regex [package name regular expression]```: list all installed versions of remaken packages whose names match a regex 
- ```remaken list [package name] [package version]```: list all files for a specific version of a remaken package 

```remaken list``` also has a ```--tree```flag. When this flag is set, ```remaken list``` displays packages informations and their dependencies tree.

### Running applications
remaken can be used to ease application run by gathering all shared libraries paths and exposing the paths in the appropriate environment variable (LD_LIBRARY_PATH for unixes, DYLD_LIBRARY_PATH for mac and PATH for windows)

```remaken run [-c debug|release] run --env [--ref remaken package reference] [--deps path_to_remaken_dependencies_description_file.txt] [--xpcf path_to_xpcf_configuration_file.xml] [--app path_to_executable] [--name ]-- [executable arguments list]```

remaken also allow to run an application installed in remaken packages structure from its package reference (i.e [package name:package version]).
```remaken run [-c debug|release] --ref [package name:package version]```
In this configuration, dynamic dependencies are searched in the package dependencies file and added to the platform library search path.

If an xpcf xml file is installed in the package folder or next to the application binary, the xpcf xml file is parsed to add the modules and their dependencies to the platform library search path.

For instance: ```remaken run -c debug --ref testxpcf:2.5.1```
The ```--ref``` option can be used with other options, especially to overwrite the application name if it differs from the package name, or to retrieve the environment set using ```--env```.

```--app``` represents the complete application filepath including the filename.

**Note** : options in **[executable arguments list]** starting with a dash (-) must be surrounded with quotes and prefixed with \ for instance **"\\-f"** to forward **-f** option to the application (this is due to CLI11 interpretation of options).

## Dependency file types
### packagedependencies dependency file

For each project, a packagedependencies.txt file can be created in the root project folder.

For dependencies specific to a particular os, a packagedependencies-[os].txt can also be created in the root project folder (os is a value in {android, linux, mac, win } ).

### extra-packages dependency file
An extra-packages.txt file can also be created along the packagedependencies.txt file.

The extra-packages.txt file can be used to add dependencies to install and bundle steps. Dependencies listed in the extra-packages file will not be used
by configure steps, or by the project build rules. 

The extra-packages purpose is to:

- install and bundle sub-dependencies needed by a project direct dependency when the direct dependency doesn't install its sub-dependency. (for instance, the conan opencv recipe doesn't install gtk+ on linux, but gtk+ is needed when a project uses opencv::imgui, hence the opencv conan recipe is "incomplete" as the imgui functionality is disabled when the gtk+ package is missing)
- install non-dependency packages : for instance install data needed by an application, configuration files ...

For packages specific to a particular os, an extra-packages-[os].txt can also be created in the root project folder (os is a value in {android, linux, mac, win } ).

## packagedependencies/extra-packages file syntax

The project build rules (builddefs-qmake for instance) will generate a packagedependencies.txt containing the build informations and will gather the dependencies in the original packagedependencies.txt and packagedependencies-[os].txt for the target os the build is run for.

### Line syntax
Each line follows the pattern :

```framework#channel | version | library name%[condition] | identifier@repository_type | repository_url | link_mode | options```

[more informations ](https://github.com/b-com-software-basis/builddefs-qmake/tree/develop#dependencies-declaration-file)


| ```framework[#channel]``` | ```version``` | ```library name%[condition]``` | ```repository_type``` | ```repository_url``` | ```link_mode``` | ```options```|
|---|---|---|---|---|---|---|
|the framework name. Optionnally the channel to get the package from |the version number|the library name| a value in: [ artifactory, nexus, github or http (http or https public repositories), vcpkg, conan, system, path : local or network filesystem root path hosting the dependencies ]|---|optional value in : [ static, shared, default (inherits the project's link mode), na (not applicable) ] |---|

**Note:**
To comment a line (and thus ignore a dependency) start the line with ```//``` (spaces and tabs before the ```//``` are ignored).

NOT IMPLEMENTED :
For artifactory, nexus and github repositories, channel is a named scope describing a common combination of compile options from the remaken packaging manifests. The combination of values become a named scope. (TODO : manage named scopes)
It is not used for other kind of repos.

### Fields specifications
| ```framework#channel``` |
|---|
|when channel is not specified, it defaults to stable for conan dependencies| 

| ```repository_type``` |
|---| 
|When repository\_type is not specified :<br>- it defaults to artifactory when identifier is either bcomBuild or thirdParties (and in this case, the identifier is also the destination subfolder where the dependencies are installed)<br>- it defaults to system when identifier is one of yum, apt, pkgtool, pkgutil, brew, macports, pacman, choco, zypperFor other repository types (github, vcpkg, conan, system) <br>When the identifier matches the repository type,the repository type reflects the identifier value - i.e. identifier = conan means repository\_type is set to conan.<br>When identifier is not specified :- @repository\_type is mandatory| 

| ```repository_url ``` |
|---| 
|repository url can be:<br>- the repository url for github, artifactory or nexus repositories<br><br>For brew, scoop, apt and conan, the repository will be added before the dependency installation.<br>For those systems, the url format is:<br>- for brew taps it will be either user/repo or user/repo#repository\_url<br>- for scoop buckets it will be either repo\_identifier or repo\_identifier#repository\_url<br>- for conan it will be repo\_identifier#repository\_url[#position]<br>- for apt it will be the ppa url<br>|

| ```options ``` |
|---| 
|options are directly forwarded to the underlying repository tool.<br>Note : to provide specific options to dedicated system packaging tools, use one line for each specific tool describing the dependency.<br>(once installed, system dependencies should not need specific options declarations during dependencies' parsing at project build stage. Hence the need for the below sample should be close to 0, except for packaging tools that build package upon install such as brew and macports and where build options can be provided).|


### Dependencies samples
| ```samples ``` |
|----------------|
|eigen \| 3.3.5 \|  eigen \|  system \|  https://github.com/SolarFramework/binaries/releases/download|
|eigen \| 3.3.5 \| eigen | brew@system | https://github.com/SolarFramework/binaries/releases/download | default | -y|
|eigen \| 3.3.5 \| eigen \| pkgtool@system \| https://github.com/SolarFramework/binaries/releases/download \| default \| --S#--noconfirm
|opencv \| 3.4.3 \| opencv \| thirdParties@github \| https://github.com/SolarFramework/binaries/releases/download|
|xpcf \| 2.1.0 \| xpcf \| bcomBuild@github \| https://github.com/SolarFramework/binaries/releases/download \| static|
|spdlog \| 0.14.0 \| spdlog \| thirdParties@github \| https://github.com/SolarFramework/binaries/releases/download|
|eigen \| 3.3.5 \| eigen \| system \||
|fbow \| 0.0.1 \| fbow \| vcpkg \||
|boost#stable \| 1.70.0 \| boost \| conan \| conan-center
|spdlog \| 0.14.0 \| spdlog \| bincrafters@conan \| conan-center \| na
|freeglut#testing \| 3.0.0 \| freeglut \| user@conan \| https://github.com/SolarFramework/binaries/releases/download|

### Packaging systems specificities
**github, artifactory, nexus and path** dependencies are installed using remaken packaging format through an url or filesystem repository.

**system** dependencies are installed using operating system dependent package manager (apt for linux debian and derivatives, brew for Mac OS X, chocolatey for windows...)

**conan** dependencies are installed using packaging format with conan package manager. conan url must match a declared remote in conan (remotes added with ```conan remote add```).

**vcpkg** dependencies are installed using vcpkg packaging format with vcpkg package manager

WARNING : using system without any OS option implies the current system the tool is run on.
Moreover, some OSes don't have a package manager, hence don't rely on system for android cross-compilation for instance.

## Remaken packaging structure

### Package tree

```
PackageName
|- PackageVersion (X.X.X)
|-- interfaces
|--- [here your header files]
|-- lib
|--- architecture (x86_64, arm64, ...)
|---- mode (shared or static)
|----- config (debug or release)
|------ [your libraries here in release mode]
|-- remaken-PackageName.pc
|-- packagedependencies.txt (if your Package requires third-parties)
|-- PackageName-PackageVersion_remakeninfo.txt
|-- .pkginfo
|--- [.headers and/or .lib, and/or bin folders]
|-- [... specific dependendy files] (wizards, csharp, xml for instance)
```

### Packaging your third parties

Remaken uses a packagedependencies.txt file defining all dependencies of your project. This file is interpreted by the QMake build script tools and is based on a simpl dependency file (.pc).
Thus, by maintaining a simple dependency file and by using the remaken package tree describing the third parties used by your project, you will be able to download the dependencies, link and build your project, and install and deploy your solution with all its dependencies in a very simple way.

Let’s describe now how to package the third parties required by your project in order to be compliant with the dependency manager and the build and install the project.

To create your package, you have to create the the package tree structure described above.

#### remaken-DependencyName.pc

This file is used by `pkg-config` to provide the necessary details for compilng and linking a program to a library.
If this file is not already provided with your third party, you will have to create it:

```
libname=PackageName
prefix=/usr/local
exec_prefix=${prefix}
libdir=${exec_prefix}/lib
includedir=${prefix}/interfaces
Name: PackageName
Description: The PackageName library
Version: PackageVersion
Requires:
Libs: -L${libdir} -l${libname}
Libs.private: ${libdir}/${pfx}${libname}.${lext}
Cflags: -I${includedir}
```

For more information concerning the syntax of this pkg-config file, you can take a look to the [pkg-config guide](https://people.freedesktop.org/~dbn/pkg-config-guide.html).

#### packagedependencies.txt

Remaken and builddefs-qmake support the recursive dependency download, link, and installation. But in order to do so, your package must precise its own dependencies in the packagedependencies.txt. 

#### PackageName-PackageVersion_remakeninfo.txt

This file is specific to remaken and builddefs-qmake, and define the platform, the C++ version and the runtime version used to build your package. This file is similar to the conaninfo.txt used by conan.

```
platform=win-cl-14.1
cppstd=17
runtime=dynamicCRT
```

#### Artifact Packager

When the previous files, your interfaces and you binaries are ready and well-placed in the folder structure detailed above, you can package the dependency by using the ArtifactPackager scripts.

In scripts folder :

	- win_artiPackager.bat
	- unixes_artiPackager.sh
	- mac_artiPackager.sh 

Just run the ArtifactPackager script for current Os at the root folder of your package, and it will automatically create folder(s) for each version :

	`architecture_mode_config`  										// 	for instance : `x86_64_shared_release` 

Each folder contains a zip file with name format :

	`PackageName_PackageVersion_architecture_mode_config.zip` 			// 	for instance : `xpcf_2.5.0_x86_64_shared_release.zip` 

This package name format must be respected and will be used by remaken to find the dependency required for a requested development environment.

#### Share your package online

In order for Remaken to find the package corresponding to the requested dependency, you have to respect the following formatting rules concerning the URL to download your package: 

	URL_root/PackageName/PackageVersion/platform/PackageName_PackageVersion_architecture_mode_config.zip

Where:

- URL_root is the URL where you host your packages (github / artifactory ...)
- platform can be win, linux, mac, android, etc.

## Building remaken
Install Qt Creator from https://www.qt.io/download

### Linux build
from the scripts folder, run ./packages.sh
It performs the needed thirdparties installations.

### Windows build


### Mac OS build
