# remaken

Remaken is a meta dependencies management tool.

It supports various several C++ packaging systems using common dependency(ies) file(s).

The dependency file follows a modular syntax.

Remaken also provide its own C++ packaging structure, based on pkg-config description files.

Remaken can be seen as the "one tool to rule them all" for C++ packagers.

## Overview

The need to use a tool such as remaken come from the diversity of package managers for C++ and to speed up development delays.

Indeed, whatever the dependency your project need, there is a chance the dependency already exists in binary format on one of the C++ package manager.

And if it doesn't exist, you can still build your own binary version and share it as a remaken dependency with your team, ensuring the build options are the same, and avoiding other developers to build locally the same dependency.

### Issues in native development without remaken and common development rules:

To setup a native C/C++ project that uses thirparty dependencies, a developer must for each dependency (whichever the build system is in either make, cmake, conan, vcpkg, VS …)  :

1. build each dependency with homogeneous build rules and flags (for instance c++ std version, for each target [debug,release | os | cpu | shared, static])
2. install each dependency in an accessible path in the system (and eventually, pollute the development environment system for instance using make install, or set a different 'sysroot')
3. add include paths, libraries paths and libraries link flags for each dependency and sub dependency in its development project
4. For each new development, even based on same dependencies : reproduce every dependency build step 1.1 to 1.3 (also true for conan when the binary hosted version doesn’t fit with your options)
5. To package a final application with its dependencies needs to bundle manually every shared dependency with the application before creating the final bundle/setup for the application

NOTE: for cmake, make, VS builds, the target can be overwritten across versions except by adding custom target path to install step

Last but not least, to run a final application : each dependency must either be copied in the same folder than the application or paths must be set to each shared dependency folder.

Shared library or application project heterogeneity across a team can lead to integration issues as the build rules can slightly differ on each developer station.

### Development workflow using remaken
Remaken :

- provide the same set of build rules across packaging systems (this is verified for packaging systems that perform a local build such as conan, vcpkg, brew)
- install dependencies from any packaging system (including remaken own packaging and flag finding system) 
- there is no need to call manually each packaging system, to write a script … all is done at once from the packagedependencies description
- build flags are populated either from remaken configure step or from builddefs-qmake rules (other rules format will come in the future, cmake, bazel ...)
- run any application with appropriate environment deduced from described dependencies and/or from xpcf configuration file
- automate dependencies bundling within an application from dependencies description (either copying external dependencies in bundle destination folder or creating a shell script for native package managers such as apt, yum ...)
- bundle xpcf applications from xpcf configuration file
- integrate conan dependencies easily without writing a conanfile.py
- provide a normalized package installation structure for remaken dependencies [sample structure]
- provide vcpkg installation and bootstrap
- provide builddefs-qmake rules installation and update
 
3/ with standardized build rules (currently qmake supported, future other system rules support can be implemented)
homogeneous builds ..
 
4/ Detailed design

## Installing remaken
### Linux or mac OS X

First install brew on your system:

```
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

Then enter the following commands from a terminal:

```
brew tap b-com/sft
brew install remaken
``` 

### Windows
Use the setup file from 

## Package formats supported
### Cross platforms packaging systems :
- Conan
- vcpkg
- brew (mac OS X and linux)
- Remaken (pkg-config based)

### Dedicated system tools :
- apt-get (debian and derivatives)
- pacman (archlinux)
- yum (redhat, fedora, centos)
- zypper (openSUSE)
- pkg (freebsd)
- pkgutil (solaris)
- chocolatey (windows)
- scoop
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

### Set remaken packages root folder
The remaken packages folder is the location where packages are installed, searched from ...
This folder defaults to ```$(HOME)/.remaken/packages```.
It can be changed in various ways:
- define the ```REMAKEN_PKG_ROOT``` environment variable to another location
- create a ```.packagespath``` file in remaken root folder  ```$(HOME)/.remaken```. In this file, provide the path to your packages folder in the first line
- provide the package path with the ```--remaken-root``` parameter from remaken command line invocation
- put the ```remaken-root``` parameter and its value in the remaken ```config``` file in located in ```$(HOME)/.remaken```

### Installing dependencies
add conditions configuration description
```remaken install [--conan_profile conan_profile_name] [-r  path_to_remaken_root] -i [-o linux] -t github [-l nexus -u http://url_to_root_nexus_repo] [--cpp-std 17] [-c debug|release] [--project_mode,-p] [path_to_remaken_dependencies_description_file.txt] ```

**Notes:**
 
- remaken_root defaults to ```$(HOME)/.remaken/packages``` or if ```REMAKEN_PKG_ROOT``` environment variable is defined, it defaults  to ```${REMAKEN_PKG_ROOT}```.
- ```remaken_dependencies_description_file``` defaults to current folder ```packagedependencies.txt```file
- ```[--project_mode,-p]```: enable project mode to generate project build files from packaging tools (conanbuildinfo.xxx, conanfile.txt ...).
   
   Project mode is enabled automatically when the folder containing the packagedependencies file also contains a QT project file
- ```[--conan_profile conan_profile_name] ``` allows to specify a specific conan profile

   When ```--conan_profile ``` is not specified:
  -  for native compilation: the ```default``` conan profile is used
  -  for cross compilation: remaken expects a ```[os]-[build-toolchain]-[architecture]``` profile to be available - for instance for android build, expects a ```android-clang-armv8``` profile
 
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

### Listing remaken installed packages
The **list** command allows to :

- ```remaken list```: list all remaken installed packages
- ```remaken list [package name]```: list all installed versions of a remaken package 
- ```remaken list --regex [package name regular expression]```: list all installed versions of remaken packages whose names match a regex 
- ```remaken list [package name] [package version]```: list all files for a specific version of a remaken package 

```remaken list``` also has a ```--tree```flag. When this flag is set, ```remaken list``` displays packages informations and their dependencies tree.

### Running applications
remaken can be used to ease application run by gathering all shared libraries paths and exposing the paths in the appropriate environment variable (LD_LIBRARY_PATH for unixes, DYLD_LIBRARY_PATH for mac and PATH for windows)

```remaken run [-c debug|release] run --env [--deps path_to_remaken_dependencies_description_file.txt] [--xpcf path_to_xpcf_configuration_file.xml] [path_to_executable] [executable arguments list]```

**Note** : options in **[executable arguments list]** starting with a dash (-) must be surrounded with quotes and prefixed with \ for instance **"\\-f"** to forward **-f** option to the application (this is due to CLI11 interpretation of options).

## Dependency file syntax

For each project, a packagedependencies.txt file can be created in the root project folder.

For dependencies specific to a particular os, a packagedependencies-[os].txt can also be created in the root project folder (os is a value in {android, linux, mac, win } ).

The project build rules (builddefs-qmake for instance) will generate a packagedependencies.txt containing the build informations and will gather the dependencies in the original packagedependencies.txt and packagedependencies-[os].txt for the target os the build is run for.

Each line follows the pattern :

```framework#channel | version | [condition]%library name | identifier@repository_type | repository_url | link_mode | options```


| ```framework#channel``` | ```version``` | ```[condition]%library name``` | ```repository_type``` | ```repository_url``` | ```link_mode``` | ```options```|
|---|---|---|---|---|---|---|
|---|---|---| a value in: [ artifactory, nexus, github or http (http or https public repositories), vcpkg, conan, system, path : local or network filesystem root path hosting the dependencies ]|---|optional value in : [ static, shared, default (inherits the project's link mode), na (not applicable) ]|---|

When link_mode is not provided :

- For remaken package format (artifactory, nexus and github), conan, system and vcpkg dependencies link_mode is set to current config link_mode

(Conan note : link_mode is mandatory if the targeted dependency needs the option. When link_mode is set to "na", it is not forwarded to conan, has some packages (typically header only libraries) don't define this option and setting the option leads to an error).

when repository_type is not specified :

- it defaults to artifactory when identifier is either bcomBuild or thirdParties (and in this case, the identifier is also the destination subfolder where the dependencies are installed)

- it defaults to system when identifier is one of yum, apt, pkgtool, pkgutil, brew, macports, pacman, choco, zypper

For other repository types (github, vcpkg, conan, system) when the identifier matches the repository type,
the repository type reflects the identifier value - i.e. identifier = conan means repository_type is set to conan.

when identifier is not specified :

- @repository_type is mandatory

repository url is the repository url. For brew repositories, the repository will be tapped before the installation. The repository url for brew can be either user/repo or user/repo#repository_url.

when channel is not specified, it defaults to stable for conan dependencies.

**Note:**
To comment a line (and thus ignore a dependency) start the line with ```//``` (spaces and tabs before the ```//``` are ignored).

NOT IMPLEMENTED :
For artifactory, nexus and github repositories, channel is a named scope describing a common combination of compile options from the remaken packaging manifests. The combination of values become a named scope. (TODO : manage named scopes)

It is not used for other kind of repos.

options are directly forwarded to the underlying repository tool.
Note : to provide specific options to dedicated system packaging tools, use one line for each specific tool describing the dependency. (once installed, system dependencies should not need specific options declarations during dependencies' parsing at project build stage. Hence the need for the below sample should be close to 0, except for packaging tools that build package upon install such as brew and macports and where build options can be provided).

For instance :

eigen | 3.3.5 | eigen | system | https://github.com/SolarFramework/binaries/releases/download

eigen | 3.3.5 | eigen | brew@system | https://github.com/SolarFramework/binaries/releases/download | default | -y

eigen | 3.3.5 | eigen | pkgtool@system | https://github.com/SolarFramework/binaries/releases/download | default | --S --noconfirm


Sample repositories declarations :

opencv | 3.4.3 | opencv | thirdParties@github | https://github.com/SolarFramework/binaries/releases/download

xpcf | 2.1.0 | xpcf | bcomBuild@github | https://github.com/SolarFramework/binaries/releases/download | static

spdlog | 0.14.0 | spdlog | thirdParties@github | https://github.com/SolarFramework/binaries/releases/download

eigen | 3.3.5 | eigen | system |

fbow | 0.0.1 | fbow | vcpkg |

boost#stable | 1.70.0 | boost | conan | conan-center

spdlog | 0.14.0 | spdlog | bincrafters@conan | conan-center | na

freeglut#testing | 3.0.0 | freeglut | user@conan | https://github.com/SolarFramework/binaries/releases/download

github, artifactory, nexus and path dependencies are installed using remaken packaging format through an url or filesystem repository.

system dependencies are installed using operating system dependent package manager (apt for linux

debian and derivatives, brew for Mac OS X, chocolatey for windows...)

conan dependencies are installed using packaging format with conan package manager. conan url must match a declared remote in conan (remotes added with ```conan remote add```).

vcpkg dependencies are installed using vcpkg packaging format with vcpkg package manager

WARNING : using system without any OS option implies the current system the tool is run on.
Moreover, some OSes don't have a package manager, hence don't rely on system for android cross-compilation for instance.

## Remaken packaging structure
### Default behavior
### Package tree
package_name/package_version/
package_name-package_version_remakeninfo.txt (or libname ??)
bcom-lib_name.pc (should be renamed to remaken-\*.pc ?)
interfaces/
lib/[arch]/[mode]/[config]/

## Building remaken
Install Qt Creator from https://www.qt.io/download

### Linux build
from the scripts folder, run ./packages.sh
It performs the needed thirdparties installations.

### Windows build


### Mac OS build
