@echo off
REM author Stephane Leduc stephane.leduc@b-com.com

REM %~dp0 = script dir path
REM %CD% = current execution path
SET workdir=%CD%

setlocal

REM Package
For /f "delims=" %%p in ('dir "%workdir%" /b /a:d') do (
	echo Package : %%p
	REM Version	
	For /f "delims=" %%v in ('dir "%workdir%\%%p" /b /a:d') do (
		echo Version : %%v
		REM platForm
		For /f "delims=" %%f in ('dir "%workdir%\%%p\%%v\lib" /b /a:d') do (
			echo Platform %%f
			REM Mode
			For /f "delims=" %%m in ('dir "%workdir%\%%p\%%v\lib\%%f" /b /a:d') do (
				echo Mode : %%m
				REM Config
				For /f "delims=" %%c in ('dir "%workdir%\%%p\%%v\lib\%%f\%%m" /b /a:d') do (
					echo Config : %%c
				
					REM Cleanup previous package
					del /s %%p_%%m_%%c\%%p_%%v_%%f_%%m_%%c.zip >nul 2>&1
					REM Create temporary package directory
					mkdir %%f_%%m_%%c\%%p\%%v
					REM Populate package directory for platform, mode and config
					REM Add interfaces
					xcopy %%p\%%v\interfaces %%f_%%m_%%c\%%p\%%v\interfaces\ /Y /S /I
					REM Add .pkginfo
					xcopy %%p\%%v\.pkginfo %%f_%%m_%%c\%%p\%%v\.pkginfo\ /Y /E /I
					REM Add CSharp
					xcopy %%p\%%v\csharp %%f_%%m_%%c\%%p\%%v\csharp\ /Y /S /I
					REM Add wizard for xpcf 
					xcopy %%p\%%v\wizards %%f_%%m_%%c\%%p\%%v\wizards\ /Y /S /I
					REM Add presets for Arcad
					xcopy %%p\%%v\presets %%f_%%m_%%c\%%p\%%v\presets\ /Y /S /I
					REM Add .pc files
					xcopy %%p\%%v\*.pc %%f_%%m_%%c\%%p\%%v\ /Y /S /I
					REM Add .txt files
					xcopy %%p\%%v\*.txt %%f_%%m_%%c\%%p\%%v\ /Y /S /I
					REM Add .xml files
					xcopy %%p\%%v\*.xml %%f_%%m_%%c\%%p\%%v\ /Y /S /I
					REM Add libraries files
					setlocal EnableDelayedExpansion
					For /f "delims=" %%t in ('dir "%workdir%\%%p\%%v\lib" /b /s /a:d') do (
						REM with local var in a loop : http://stackoverflow.com/questions/8648178/how-to-get-substring-of-a-token-in-for-loop
						REM find substring : http://stackoverflow.com/questions/7005951/batch-file-find-if-substring-is-in-string-not-in-a-file
						SET path=%%t
						REM try to replace %%f\%%\%%c by empty then check the 2 strings
						SET checkPath=!path:%%f\%%m\%%c=!
						
						if not !Path!==!checkPath! (
							set toplibdir=!path:%workdir%=!
							mkdir %%f_%%m_%%c\!toplibdir!
							copy !Path!\. %%f_%%m_%%c\!toplibdir!
						)						
					)
					endlocal
					
					REM Prepare package archive
					cd  %%f_%%m_%%c
					7z a -tzip %%p_%%v_%%f_%%m_%%c.zip %%p
					cd ..
					REM Cleanup temporary directory
					rmdir /S /Q %%f_%%m_%%c\%%p
				)	
			)
		)
	)		
)

goto:eof

:contains




