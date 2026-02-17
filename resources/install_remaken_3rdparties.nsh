;============================================================================================================
; sleduc - allow to search string in %PATH% for detect if cmake is in system PATH
;============================================================================================================
; StrContains
; This function does a case sensitive searches for an occurrence of a substring in a string. 
; It returns the substring if it is found. 
; Otherwise it returns null(""). 
; Written by kenglish_hi
; Adapted from StrReplace written by dandaman32
  
Var STR_HAYSTACK
Var STR_NEEDLE
Var STR_CONTAINS_VAR_1
Var STR_CONTAINS_VAR_2
Var STR_CONTAINS_VAR_3
Var STR_CONTAINS_VAR_4
Var STR_RETURN_VAR
 
Function StrContains
  Exch $STR_NEEDLE
  Exch 1
  Exch $STR_HAYSTACK
  ; Uncomment to debug
  ;MessageBox MB_OK 'STR_NEEDLE = $STR_NEEDLE STR_HAYSTACK = $STR_HAYSTACK '
    StrCpy $STR_RETURN_VAR ""
    StrCpy $STR_CONTAINS_VAR_1 -1
    StrLen $STR_CONTAINS_VAR_2 $STR_NEEDLE
    StrLen $STR_CONTAINS_VAR_4 $STR_HAYSTACK
    loop:
      IntOp $STR_CONTAINS_VAR_1 $STR_CONTAINS_VAR_1 + 1
      StrCpy $STR_CONTAINS_VAR_3 $STR_HAYSTACK $STR_CONTAINS_VAR_2 $STR_CONTAINS_VAR_1
      StrCmp $STR_CONTAINS_VAR_3 $STR_NEEDLE found
      StrCmp $STR_CONTAINS_VAR_1 $STR_CONTAINS_VAR_4 done
      Goto loop
    found:
      StrCpy $STR_RETURN_VAR $STR_NEEDLE
      Goto done
    done:
   Pop $STR_NEEDLE ;Prevent "invalid opcode" errors and keep the
   Exch $STR_RETURN_VAR  
FunctionEnd
 
!macro _StrContainsConstructor OUT NEEDLE HAYSTACK
  Push `${HAYSTACK}`
  Push `${NEEDLE}`
  Call StrContains
  Pop `${OUT}`
!macroend
 
!define StrContains '!insertmacro "_StrContainsConstructor"'

;============================================================================================================


; REMAKEN_PKG_ROOT first path defintion
Var remaken_pkg_root_text
Var xpcf_module_root_text

#install - add env var
Section "!Remaken"	REMAKEN_APP
	EnVar::SetHKCU
	# check with different default values
	StrCmp $remaken_pkg_root_text '' default_remaken_pkg_root
	StrCmp $remaken_pkg_root_text "$PROFILE" default_remaken_pkg_root
	StrCmp $remaken_pkg_root_text "$PROFILE\.remaken" default_remaken_pkg_root
		# add env var if custom value
		EnVar::AddValue "REMAKEN_PKG_ROOT" "$remaken_pkg_root_text"
		Goto end_remaken_pkg_root		
	default_remaken_pkg_root:
		# remove env var if default value
		EnVar::Delete "REMAKEN_PKG_ROOT"
	end_remaken_pkg_root:		
SectionEnd

Section "XPCF_MODULE_ROOT"	XPCF_MODULE_ENVVAR
	EnVar::SetHKCU
	EnVar::AddValue "XPCF_MODULE_ROOT" "$xpcf_module_root_text"
SectionEnd

#uninstall - delete env var
Section "!un.Remaken"
	EnVar::SetHKCU
	EnVar::Delete "REMAKEN_PKG_ROOT"
SectionEnd

Section "un.XPCF_MODULE_ROOT"
	EnVar::SetHKCU
	EnVar::Delete "XPCF_MODULE_ROOT"
SectionEnd

# write .packagespath during install
Section
	# check with different default values
	StrCmp $remaken_pkg_root_text '' default_remaken_pkg_root
	StrCmp $remaken_pkg_root_text "$PROFILE" default_remaken_pkg_root
	StrCmp $remaken_pkg_root_text "$PROFILE\.remaken" default_remaken_pkg_root
		# write file if custom value
		FileOpen $9 $PROFILE\.remaken\.packagespath w
		FileWrite $9 "$remaken_pkg_root_text"
		FileClose $9 
		Goto end_remaken_pkg_root
	default_remaken_pkg_root:
		# remove file if default value
		Delete $PROFILE\.remaken\.packagespath
	end_remaken_pkg_root:
SectionEnd	

var python_install_dir
Var conan1Selected
Var conan2Selected
SectionGroup /e "Chocolatey" CHOCO_TOOLS
	Section "-hidden readEnvVar"
		ReadEnvStr $1 SYSTEMDRIVE
	SectionEnd
	Section "-hidden InstallChoco"	INSTALL_CHOCO
		IfFileExists "$1\ProgramData\Chocolatey\choco.exe" choco_exists
			${PowerShellExec} "Set-ExecutionPolicy Bypass -Scope Process -Force; iex ((New-Object System.Net.WebClient).DownloadString('https://chocolatey.org/install.ps1'))"  
			Pop $R1 ;$R1 is "hello powershell"
		choco_exists:
	SectionEnd
	
	Section "7Zip"	CHOCO_TOOLS_7ZIP
		IfFileExists "$1\ProgramData\chocolatey\bin\7z.exe" sevenz_exists
		ExecWait '$1\ProgramData\Chocolatey\choco install -yr --acceptlicense  --no-progress 7zip'
		sevenz_exists:
	SectionEnd
	
	Section "Curl"	CHOCO_TOOLS_CURL
		IfFileExists "$1\ProgramData\chocolatey\bin\curl.exe" curl_exists
		ExecWait '$1\ProgramData\Chocolatey\choco install -yr --acceptlicense  --no-progress curl'
		curl_exists:
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

			ExecWait '$0 -m pip install --upgrade pip'
		;pip_exists:
		
		${If} $conan1Selected == 1
			ExecWait '$python_install_dir\Scripts\pip install --upgrade conan==1.64.0'
			ExecWait '$python_install_dir\Scripts\conan config set general.revisions_enabled=0'
		${Else}
			ExecWait '$python_install_dir\Scripts\pip install --upgrade conan'
		${EndIf}
		
	SectionEnd	
	Section "pkg-config" CHOCO_TOOLS_PKG_CONFIG
		ExecWait '$1\ProgramData\Chocolatey\choco install -yr --acceptlicense --no-progress pkgconfiglite'
	SectionEnd
	Section "CMake" CHOCO_TOOLS_CMAKE
		ReadRegStr $0 HKLM "SOFTWARE\Kitware\CMake" "InstallDir"
		${IfNot} ${Errors}
			SetRegView 64
			ReadRegStr $2 HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment" "Path"
			ReadRegStr $3 HKCU "Environment" "Path"
			StrCpy $2 "$2_$3"
			${StrContains} $1 "$0" "$2"
			StrCmp $1 "" cmakenotfound
			  #MessageBox MB_OK 'Found string $1 in $%PATH% \n then install normally'
			  Goto cmakedone
			cmakenotfound:
			  #MessageBox MB_OK 'Did not find string $1 - $0 then remove before install'
			  ExecWait '$1\ProgramData\Chocolatey\choco uninstall -yr --acceptlicense --no-progress CMake cmake.install'
			cmakedone:
		${EndIf}	
		ExecWait '$1\ProgramData\Chocolatey\choco install -yr --acceptlicense --no-progress CMake --installargs $\'ADD_CMAKE_TO_PATH=System$\''
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
	SectionSetFlags ${CHOCO_TOOLS_CURL} 17
	
	# disable for test in dev
	#SectionSetFlags ${CONAN} 0
	#SectionSetFlags ${CHOCO_TOOLS_PKG_CONFIG} 0
	#SectionSetFlags ${CHOCO_TOOLS_CMAKE} 0
	
	#read current REMAKEN_PKG_ROOT env var
	ReadEnvStr $0 REMAKEN_PKG_ROOT
	StrCpy $remaken_pkg_root_text $0
	
	#read current XPCF_MODULE_ROOT env var
	ReadEnvStr $0 XPCF_MODULE_ROOT
	StrCpy $xpcf_module_root_text $0
FunctionEnd
!endif

!ifdef CUSTOMIZE_UNONINIT
Function un.CustomizeUnOnInit
	#read current REMAKEN_PKG_ROOT env var
	ReadEnvStr $0 REMAKEN_PKG_ROOT
	StrCpy $remaken_pkg_root_text $0
	
	#read current XPCF_MODULE_ROOT env var
	ReadEnvStr $0 XPCF_MODULE_ROOT
	StrCpy $xpcf_module_root_text $0
FunctionEnd
!endif

!ifdef CUSTOMIZE_DISPLAY_PAGE_COMPONENTS
; Section descriptions
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${REMAKEN_APP} "Install Remaken Tool"
  !insertmacro MUI_DESCRIPTION_TEXT ${XPCF_MODULE_ENVVAR} "Configure and create XPCF_MODULE_ROOT environment variable"
  !insertmacro MUI_DESCRIPTION_TEXT ${CHOCO_TOOLS} "Remaken use Chocolatey as system install tool"
  !insertmacro MUI_DESCRIPTION_TEXT ${CHOCO_TOOLS_7ZIP} "Remaken use 7zip as system file compression/extraction tool"
  !insertmacro MUI_DESCRIPTION_TEXT ${CHOCO_TOOLS_CURL} "Remaken use Curl as system file download tool"
  !insertmacro MUI_DESCRIPTION_TEXT ${CONAN} "Remaken can use Conan dependencies (Conan will be installed with Python and Pip also installed)"
  !insertmacro MUI_DESCRIPTION_TEXT ${CHOCO_TOOLS_PKG_CONFIG} "Remaken provides its own C++ packaging structure, based on pkg-config description files (pkg-config will be installed with Choco)"
  !insertmacro MUI_DESCRIPTION_TEXT ${CHOCO_TOOLS_CMAKE} "Conan can uses CMake for build packages (CMake will be installed with Choco)"
!insertmacro MUI_FUNCTION_DESCRIPTION_END
!endif


!ifdef CUSTOMIZE_ADD_CUSTOM_PAGE


Function custompage_conanversion

	#https://nsis.sourceforge.io/Show_custom_page_when_a_section_has_been_selected
	SectionGetFlags ${CONAN} $R0 
	IntOp $R0 $R0 & ${SF_SELECTED} 
	IntCmp $R0 ${SF_SELECTED} show 
 	Abort 
	show:	
	
	!insertmacro MUI_HEADER_TEXT "Define conan version" ""
	nsDialogs::Create 1018
	Pop $0
	${NSD_CreateLabel} 0 28u 100% 12u "Select Conan version :"
	${NSD_CreateRadioButton} 12u 44u 100% 12u "Install Conan 1 (1.64.0)"
	pop $1
	${NSD_CreateRadioButton} 12u 60u 100% 12u "Install Conan 2 (last version)"
	pop $2
	${NSD_Check} $2 ; Select conan 2 radio button
	nsDialogs::Show
	
FunctionEnd

Function custompage_conanversion_leave
	${NSD_GetState} $1 $conan1Selected
	${NSD_GetState} $2 $conan2Selected
FunctionEnd


Var TextBox
Var BrowseButton
Var NextButton
Var Label
Var remaken_pkg_root_text_display
Function custompage_remakenroot 
	
	#https://nsis.sourceforge.io/Show_custom_page_when_a_section_has_been_selected
	SectionGetFlags ${XPCF_MODULE_ENVVAR} $R0 
	IntOp $R0 $R0 & ${SF_SELECTED} 
	IntCmp $R0 ${SF_SELECTED} show 
 	Abort 
	show:	

	; set empty if equals $PROFILE - manage remaken_pkg_root_text_display for display information
	
	StrCpy $remaken_pkg_root_text_display $remaken_pkg_root_text
	StrCmp $remaken_pkg_root_text '' notdefined_remaken_pkg_root_text 
	StrCmp $remaken_pkg_root_text "$PROFILE" clear_remaken_pkg_root_text continue 
	StrCmp $remaken_pkg_root_text "$PROFILE\.remaken" clear_remaken_pkg_root_text continue
	clear_remaken_pkg_root_text:
		StrCpy $remaken_pkg_root_text ''
		StrCpy $remaken_pkg_root_text_display "Default value (then removed when setup Remaken)"
		Goto continue
	notdefined_remaken_pkg_root_text:
		StrCpy $remaken_pkg_root_text ''
		StrCpy $remaken_pkg_root_text_display "Not defined - use default install path"
	continue:
	
	!insertmacro MUI_HEADER_TEXT "Define where to install Remaken packages" ""
    nsDialogs::Create 1018
    Pop $0
    ${NSD_CreateLabel} 0 0u 100% 12u "By default, Remaken packages are installed in :"
	${NSD_CreateLabel} 12u 12u 100% 12u "$PROFILE\.remaken\packages"
	${NSD_CreateLabel} 0u 28u 100% 12u "A custom path can be defined saved in REMAKEN_PKG_ROOT env var"
	${NSD_CreateLabel} 0u 44u 100% 12u "Current REMAKEN_PKG_ROOT : $remaken_pkg_root_text_display"
	${NSD_CreateLabel} 0u 60u 100% 12u "You can select a 'Remaken packages custom path' (set/keep empty for default behaviour) :"
		
	Pop $0
	${NSD_CreateDirRequest} 0 72u 84% 12u "$remaken_pkg_root_text"
    Pop $TextBox
    ${NSD_SetText} $TextBox $remaken_pkg_root_text
    
	${NSD_CreateBrowseButton} 85% 72u 15% 12u "Browse"
    Pop $BrowseButton
    ${NSD_OnClick} $BrowseButton OnBrowseForDir_remakenroot
	
	${NSD_CreateLabel} 0u 102u 100% 12u "Note : please run 'remaken init' to install qmake rules"
	Pop $Label
	CreateFont $0 "Arial" 14
	SendMessage $Label ${WM_SETFONT} $0 1
	
	GetDlgItem $NextButton $HWNDPARENT 1 ; next=1, cancel=2, back=315
	System::Call shlwapi::SHAutoComplete(p$TextBox,i${SHACF_FILESYSTEM})
	
	nsDialogs::Show
	
FunctionEnd

Function custompage_remakenroot_leave
	${NSD_GetText} $TextBox $remaken_pkg_root_text
FunctionEnd

Function OnBrowseForDir_remakenroot
	${NSD_GetText} $TextBox $remaken_pkg_root_text
    nsDialogs::SelectFolderDialog /NOUNLOAD "Directory" $remaken_pkg_root_text
    Pop $0
    ${If} $0 == error
    ${Else}
        StrCpy $remaken_pkg_root_text $0
        ${NSD_SetText} $TextBox $remaken_pkg_root_text
    ${EndIf}
FunctionEnd

Function custompage_xpcfmoduleroot 
	
	;if empty use $PROFILE else currentdir read in registry
	StrCmp $xpcf_module_root_text '' 0 lbl_xpcfcurrentdir
	StrCpy $xpcf_module_root_text "$PROFILE\.remaken\packages\win-cl-14.1\"
	lbl_xpcfcurrentdir:
		
	!insertmacro MUI_HEADER_TEXT "Define XPCF_MODULE_ROOT environment variable" ""
    nsDialogs::Create 1018
    Pop $0
    ${NSD_CreateLabel} 0 0u 100% 12u "When not overriden in a previous install, it defaults to :"
	${NSD_CreateLabel} 12u 12u 100% 12u "$PROFILE\.remaken\packages\win-cl_14.1"
	${NSD_CreateLabel} 0u 28u 100% 12u "Current XPCF_MODULE_ROOT value is : $xpcf_module_root_text"
	${NSD_CreateLabel} 0u 44u 100% 12u "Please select XPCF_MODULE_ROOT path :"
		
	Pop $0
	${NSD_CreateDirRequest} 0 56u 84% 12u "$xpcf_module_root_text"
    Pop $TextBox
    ${NSD_SetText} $TextBox $xpcf_module_root_text
    
	${NSD_CreateBrowseButton} 85% 56u 15% 12u "Browse"
    Pop $BrowseButton
    ${NSD_OnClick} $BrowseButton OnBrowseFor_xpcfmoduleroot	
	
	GetDlgItem $NextButton $HWNDPARENT 1 ; next=1, cancel=2, back=315
	${NSD_OnChange} $TextBox OnValidate_xpcfmoduleroot
	
	System::Call shlwapi::SHAutoComplete(p$TextBox,i${SHACF_FILESYSTEM})
	
	nsDialogs::Show
	
FunctionEnd

Function custompage_xpcfmoduleroot_leave
	${NSD_GetText} $TextBox $xpcf_module_root_text
FunctionEnd

Function OnBrowseFor_xpcfmoduleroot	
	${NSD_GetText} $TextBox $xpcf_module_root_text
    nsDialogs::SelectFolderDialog /NOUNLOAD "Directory" $xpcf_module_root_text
    Pop $0
    ${If} $0 == error
    ${Else}
        StrCpy $xpcf_module_root_text $0
        ${NSD_SetText} $TextBox $xpcf_module_root_text
    ${EndIf}
FunctionEnd

; check valid path
Function OnValidate_xpcfmoduleroot	
	${NSD_GetText} $TextBox $xpcf_module_root_text
	StrCpy $R2 $xpcf_module_root_text 1 2
	StrCmp $R2 "\" PathOK
	EnableWindow $NextButton 0 ;disable next/install button
	goto OnValidateXpcfPathEnd
	
	PathOK:
		EnableWindow $NextButton 1 ;enable next/install button
	OnValidateXpcfPathEnd:
FunctionEnd

!endif


