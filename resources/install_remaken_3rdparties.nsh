; REMAKEN_PKG_ROOT first path defintion
Var remaken_pkg_root_text

Section "!Remaken"	REMAKEN_APP
	EnVar::SetHKCU
	EnVar::AddValue "REMAKEN_PKG_ROOT" "$remaken_pkg_root_text\.remaken"
SectionEnd

Section "!un.Remaken"
	EnVar::SetHKCU
	EnVar::Delete "REMAKEN_PKG_ROOT"
SectionEnd

Section
	FileOpen $9 $PROFILE\.remaken\.packagespath w
	FileWrite $9 "$remaken_pkg_root_text\.remaken"
	FileClose $9 
SectionEnd	

var python_install_dir
SectionGroup /e "Chocolatey" CHOCO_TOOLS
	Section "-hidden InstallChoco"	INSTALL_CHOCO
		IfFileExists "$0\ProgramData\Chocolatey\choco.exe" choco_exists
		${PowerShellExec} "Set-ExecutionPolicy Bypass -Scope Process -Force; iex ((New-Object System.Net.WebClient).DownloadString('https://chocolatey.org/install.ps1'))"  
		Pop $R1 ;$R1 is "hello powershell"
		choco_exists:
	SectionEnd
	Section "-hidden readEnvVar"
		ReadEnvStr $1 SYSTEMDRIVE
	SectionEnd
	Section "7Zip"	CHOCO_TOOLS_7ZIP
		IfFileExists "$1\ProgramData\chocolatey\bin\7z.exe" sevenz_exists
		ExecWait '$1\ProgramData\Chocolatey\choco install -yr --acceptlicense  --no-progress 7zip'
		sevenz_exists:
	SectionEnd
	Section "Conan (with python/pip)" CONAN 
		StrCpy $python_install_dir $1\Python37	; default dir
		ReadRegStr $0 HKLM "SOFTWARE\Python\PythonCore\3.7\InstallPath" ""
		${IfNot} ${Errors}
			StrCpy $python_install_dir $0
			; remove last '\'
			StrCpy $2 "$0" "" -1 ; this gets the last char
			StrCmp $2 "\" 0 +2 ; check if last char is '\'
			StrCpy $python_install_dir "$0" -1 ; last char was '\', remove it
			ReadRegStr $0 HKLM "SOFTWARE\Python\PythonCore\3.7\InstallPath" "ExecutablePath"
		${EndIf}
		;install/update python only if exe not found
		IfFileExists $0 python_exists
			ExecWait '$1\ProgramData\Chocolatey\choco install -yr --force --acceptlicense --no-progress python3 --version=3.7.3 --params $\"/InstallDir:$python_install_dir$\"'
		python_exists:

		;always install/update pip
		;IfFileExists $1\Python3\Scripts\pip.exe pip_exists
			ExecWait '$0 -m pip install --upgrade pip'
		;pip_exists:
		
		;always install/update conan
		;IfFileExists $1\Python3\Scripts\conan.exe conan_exists
			ExecWait '$python_install_dir\Scripts\pip install --upgrade conan'
		;conan_exists:
	SectionEnd	
	Section "pkg-config" CHOCO_TOOLS_PKG_CONFIG
		ExecWait '$1\ProgramData\Chocolatey\choco install -yr --acceptlicense --no-progress pkgconfiglite'
	SectionEnd
	Section "CMake" CHOCO_TOOLS_CMAKE
		ExecWait '$1\ProgramData\Chocolatey\choco install -yr --acceptlicense --no-progress CMake'
	SectionEnd	
		
	Section "-hidden Install_redist"
		SetRegView 64
		ReadRegStr $0 HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\{F7CAC7DF-3524-4C2D-A7DB-E16140A3D5E6}" "DisplayName"
		${If} ${Errors}
			ExecWait '$1\ProgramData\Chocolatey\choco install -yr --acceptlicense  --no-progress vcredist2017'
		${EndIf}
	SectionEnd
SectionGroupEnd

!ifdef CUSTOMIZE_ONINIT
Function CustomizeOnInit
	; enable/expand... section items
    SectionSetFlags ${REMAKEN_APP} 17 ; selected and ReadOnly
    SectionSetFlags ${CHOCO_TOOLS} 51   ; selected, ReadOnly and expanded
	SectionSetFlags ${CHOCO_TOOLS_7ZIP} 17
	
	; manage custom REMAKEN_PKG_ROOT
	SetRegView 64
	ReadRegStr $0 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${SETUP_GUID}" "current_remaken_pkg_root"
	StrCpy $remaken_pkg_root_text $0
FunctionEnd
!endif

!ifdef CUSTOMIZE_UNONINIT
Function un.CustomizeUnOnInit
	; manage custom REMAKEN_PKG_ROOT
	SetRegView 64
	ReadRegStr $0 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${SETUP_GUID}" "current_remaken_pkg_root"
	StrCpy $remaken_pkg_root_text $0
FunctionEnd
!endif

!ifdef CUSTOMIZE_DISPLAY_PAGE_COMPONENTS
; Section descriptions
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${REMAKEN_APP} "Install Remaken Tool"
  !insertmacro MUI_DESCRIPTION_TEXT ${CHOCO_TOOLS} "Remaken use Chocolatey as system install tool"
  !insertmacro MUI_DESCRIPTION_TEXT ${CHOCO_TOOLS_7ZIP} "Remaken use 7zip as system file compression/extraction tool"
  !insertmacro MUI_DESCRIPTION_TEXT ${CONAN} "Remaken can use Conan dependencies (Conan will be installed with Python and Pip also installed)"
  !insertmacro MUI_DESCRIPTION_TEXT ${CHOCO_TOOLS_PKG_CONFIG} "Remaken provides its own C++ packaging structure, based on pkg-config description files (pkg-config will be installed with Choco)"
  !insertmacro MUI_DESCRIPTION_TEXT ${CHOCO_TOOLS_CMAKE} "Conan can uses CMake for build packages (CMake will be installed with Choco)"
!insertmacro MUI_FUNCTION_DESCRIPTION_END
!endif


!ifdef CUSTOMIZE_ADD_CUSTOM_PAGE
Var TextBox
Var BrowseButton
Var NextButton
Var Label
Function custompage_remakenroot 
	
	;if empty use $PROFILE else currentdir read in registry
	StrCmp $remaken_pkg_root_text '' 0 lbl_currentdir
	StrCpy $remaken_pkg_root_text "$PROFILE"
	lbl_currentdir:
		
	!insertmacro MUI_HEADER_TEXT "Define where to install Remaken packages" ""
    nsDialogs::Create 1018
    Pop $0
    ;${NSD_CreateLabel} 0 0 100% 12u "Define where to install Remaken packages : "
	${NSD_CreateLabel} 0 0u 100% 12u "When not overrided in a previous install, it defaults to :"
	${NSD_CreateLabel} 12u 12u 100% 12u "$PROFILE (+ \.remaken\packages)"
	${NSD_CreateLabel} 0u 28u 100% 12u "Current Remaken packages 'root path' is : $remaken_pkg_root_text"
	${NSD_CreateLabel} 0u 44u 100% 12u "Please select Remaken packages 'root path' (named below PKG_ROOT_PATH) :"
		
	Pop $0
	${NSD_CreateDirRequest} 0 56u 84% 12u "$remaken_pkg_root_text"
    Pop $TextBox
    ${NSD_SetText} $TextBox $remaken_pkg_root_text
    
	${NSD_CreateBrowseButton} 85% 56u 15% 12u "Browse"
    Pop $BrowseButton
    ${NSD_OnClick} $BrowseButton OnBrowseForDir	
	
	${NSD_CreateLabel} 0u 74u 100% 12u "REMAKEN_PKG_ROOT env var is PKG_ROOT_PATH\.remaken"
	
	${NSD_CreateLabel} 0u 102u 100% 12u "Note : please run 'remaken init' to install qmake rules"
	Pop $Label
	CreateFont $0 "Arial" 14
	SendMessage $Label ${WM_SETFONT} $0 1
	

	GetDlgItem $NextButton $HWNDPARENT 1 ; next=1, cancel=2, back=315
	${NSD_OnChange} $TextBox OnValidatePath
	
	System::Call shlwapi::SHAutoComplete(p$TextBox,i${SHACF_FILESYSTEM})
	
	nsDialogs::Show
	
FunctionEnd

Function custompage_remakenroot_leave
	${NSD_GetText} $TextBox $remaken_pkg_root_text
	SetRegView 64
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${SETUP_GUID}" "current_remaken_pkg_root" "$remaken_pkg_root_text"
FunctionEnd

Function OnBrowseForDir
	${NSD_GetText} $TextBox $remaken_pkg_root_text
    nsDialogs::SelectFolderDialog /NOUNLOAD "Directory" $remaken_pkg_root_text
    Pop $0
    ${If} $0 == error
    ${Else}
        StrCpy $remaken_pkg_root_text $0
        ${NSD_SetText} $TextBox $remaken_pkg_root_text
    ${EndIf}
FunctionEnd

; check valid path
Function OnValidatePath	
	${NSD_GetText} $TextBox $remaken_pkg_root_text
	StrCpy $R2 $remaken_pkg_root_text 1 2
	StrCmp $R2 "\" PathOK
	EnableWindow $NextButton 0 ;disable next/install button
	goto OnValidatePathEnd
	
	PathOK:
		EnableWindow $NextButton 1 ;enable next/install button
	OnValidatePathEnd:
FunctionEnd


!endif


