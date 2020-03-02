; remaken_root first path defintion
Var remaken_root_text

Section "!Remaken"	REMAKEN_APP
	SetRegView 64
	WriteRegStr HKCU "Environment" "REMAKEN_ROOT" "$remaken_root_text\.remaken"
SectionEnd

Section "!un.Remaken"
	SetRegView 64
	DeleteRegValue HKCU "Environment" "REMAKEN_ROOT"
SectionEnd

Section "!BuilddefsQmake"   BUILDDEFS_QMAKE
    SetOutPath "$remaken_root_text\.remaken\rules"
    CreateDirectory $remaken_root_text\.remaken\rules
    File /nonfatal /a /r "${SETUP_PROJECT_PATH}\Builddefs\"
	SetRegView 64
	WriteRegStr HKCU "Environment" "REMAKEN_RULES_ROOT" "$remaken_root_text\.remaken\rules"
SectionEnd

Section "un.BuilddefsQmake"
    RMDir /r "$remaken_root_text\.remaken\rules"
	RMDir "$remaken_root_text\.remaken"
	RMDir "$remaken_root_text"
	SetRegView 64
	DeleteRegValue HKCU "Environment" "REMAKEN_RULES_ROOT"
SectionEnd


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
                IfFileExists $1\Python37\python.exe python_exists
                ExecWait '$1\ProgramData\Chocolatey\choco install -yr --force --acceptlicense  --no-progress python3 --params $\"/InstallDir:$1\Python37$\"'
		python_exists:

                ;IfFileExists $1\Python3\Scripts\pip.exe pip_exists
                ExecWait '$1\Python3\python -m pip install --upgrade pip'
		;pip_exists:
		
                ;IfFileExists $1\Python3\Scripts\conan.exe conan_exists
                ExecWait '$1\Python3\Scripts\pip install --upgrade conan'
		;conan_exists:
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
    SectionSetFlags ${BUILDDEFS_QMAKE} 1 ; selected
    SectionSetFlags ${CHOCO_TOOLS} 51   ; selected, ReadOnly and expanded
	SectionSetFlags ${CHOCO_TOOLS_7ZIP} 17
	
	; manage custom REMAKEN_ROOT
	SetRegView 64
	ReadRegStr $0 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${SETUP_GUID}" "current_remaken_root"
	StrCpy $remaken_root_text $0
FunctionEnd
!endif

!ifdef CUSTOMIZE_UNONINIT
Function un.CustomizeUnOnInit
	
	; manage custom REMAKEN_ROOT
	SetRegView 64
	ReadRegStr $0 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${SETUP_GUID}" "current_remaken_root"
	StrCpy $remaken_root_text $0
FunctionEnd
!endif

!ifdef CUSTOMIZE_DISPLAY_PAGE_COMPONENTS
; Section descriptions
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${REMAKEN_APP} "Install Remaken Tool"
  !insertmacro MUI_DESCRIPTION_TEXT ${BUILDDEFS_QMAKE} "Install Builddefs/qmake"
  !insertmacro MUI_DESCRIPTION_TEXT ${CHOCO_TOOLS} "Remaken use Chocolatey as system install tool"
  !insertmacro MUI_DESCRIPTION_TEXT ${CHOCO_TOOLS_7ZIP} "Remaken use 7zip as system file compression/extraction tool"
  !insertmacro MUI_DESCRIPTION_TEXT ${CONAN} "Remaken can use Conan dependencies (Conan will be installed with Python and Pip also installed)"
!insertmacro MUI_FUNCTION_DESCRIPTION_END
!endif


!ifdef CUSTOMIZE_ADD_CUSTOM_PAGE
Var TextBox
Var BrowseButton
Var NextButton
Function custompage_remakenroot 
	
	;if empty use $PROFILE else currentdir read in registry
	StrCmp $remaken_root_text '' 0 lbl_currentdir
	StrCpy $remaken_root_text "$PROFILE"
	lbl_currentdir:
		
	!insertmacro MUI_HEADER_TEXT "Define REMAKEN_ROOT environment variable" ""
    nsDialogs::Create 1018
    Pop $0
    ${NSD_CreateLabel} 0 0 100% 12u "Define the 'first part' of REMAKEN_ROOT environment variable"
	${NSD_CreateLabel} 0 12u 100% 12u "by default (if not overrided in previous install) :"
	${NSD_CreateLabel} 12u 24u 100% 12u "- REMAKEN_ROOT is $PROFILE (+ \.remaken)"
	${NSD_CreateLabel} 12u 36u 100% 12u "- REMAKEN_RULES_ROOT is $PROFILE (+ \.remaken\rules)"
	${NSD_CreateLabel} 0u 60u 100% 12u "Current REMAKEN_ROOT is :"
	Pop $0
	${NSD_CreateDirRequest} 0 78u 84% 12u "$remaken_root_text"
    Pop $TextBox
    ${NSD_SetText} $TextBox $remaken_root_text
    
	${NSD_CreateBrowseButton} 85% 78u 15% 12u "Browse"
    Pop $BrowseButton
    ${NSD_OnClick} $BrowseButton OnBrowseForDir	
	
	GetDlgItem $NextButton $HWNDPARENT 1 ; next=1, cancel=2, back=315
	${NSD_OnChange} $TextBox OnValidatePath
	
	System::Call shlwapi::SHAutoComplete(p$TextBox,i${SHACF_FILESYSTEM})
	
	nsDialogs::Show
	
FunctionEnd

Function custompage_remakenroot_leave
	${NSD_GetText} $TextBox $remaken_root_text
	SetRegView 64
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${SETUP_GUID}" "current_remaken_root" "$remaken_root_text"
FunctionEnd

Function OnBrowseForDir
	${NSD_GetText} $TextBox $remaken_root_text
    nsDialogs::SelectFolderDialog /NOUNLOAD "Directory" $remaken_root_text
    Pop $0
    ${If} $0 == error
    ${Else}
        StrCpy $remaken_root_text $0
        ${NSD_SetText} $TextBox $remaken_root_text
    ${EndIf}
FunctionEnd

; check valid path
Function OnValidatePath	
	${NSD_GetText} $TextBox $remaken_root_text
	StrCpy $R2 $remaken_root_text 1 2
	StrCmp $R2 "\" PathOK
	EnableWindow $NextButton 0 ;disable next/install button
	goto OnValidatePathEnd
	
	PathOK:
		EnableWindow $NextButton 1 ;enable next/install button
	OnValidatePathEnd:
FunctionEnd


!endif


