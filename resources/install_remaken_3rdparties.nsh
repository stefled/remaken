Section "!Remaken"	REMAKEN_APP
SectionEnd

Section "!BuilddefsQmake"   BUILDDEFS_QMAKE
    SetOutPath "$PROFILE\.remaken\rules"
    CreateDirectory $PROFILE\.remaken\rules
    File /nonfatal /a /r "${SETUP_PROJECT_PATH}\Builddefs\"
	WriteRegExpandStr HKCU "Environment" REMAKEN_RULES_ROOT $PROFILE\.remaken\rules
SectionEnd

Section "un.BuilddefsQmake"
    RMDir /r "$PROFILE\.remaken\rules"
	
	 DeleteRegValue HKCU "Environment" REMAKEN_RULES_ROOT
SectionEnd


SectionGroup /e "Chocolatey" CHOCO_TOOLS
	Section "-hidden InstallChoco"	INSTALL_CHOCO
		IfFileExists "$0\ProgramData\Chocolatey\choco.exe" choco_exists
		#IfFileExists "choco" choco_exists
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
                IfFileExists $1\Python3\python.exe python_exists
                ExecWait '$1\ProgramData\Chocolatey\choco install -yr --force --acceptlicense  --no-progress python3 --params $\"/InstallDir:$1\Python3$\"'
		python_exists:

                IfFileExists $1\Python3\Scripts\pip.exe pip_exists
                ExecWait '$1\Python3\python -m pip install --upgrade pip'
		pip_exists:
		
                IfFileExists $1\Python3\Scripts\conan.exe conan_exists
                ExecWait '$1\Python3\Scripts\pip install conan'
		conan_exists:
	SectionEnd
	Section "-hidden Install_redist"
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
