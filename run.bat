:: Run "run.bat r" to remove the data directory before running rl.exe

@echo off
call build.bat
if %errorLevel%==0 (
	cd bin
	if "%~1"=="r" (
		del /Q data
	)
	call rl.exe
	cd ..
)
