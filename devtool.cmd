@echo off
setlocal
rem VKE DevTool launcher (Windows). Bootstraps Python if missing, then (re)starts the server so code changes are always loaded.
set "DEVTOOL_DIR=%~dp0tools\devtool\"
set "PY="

where py >nul 2>nul && set "PY=py -3"
if not defined PY (
	where python >nul 2>nul && python -c "import sys; sys.exit(0 if sys.version_info >= (3,9) else 1)" >nul 2>nul && set "PY=python"
)

if not defined PY (
	echo [devtool] Python 3.9+ not found. Installing via winget...
	where winget >nul 2>nul || (
		echo [devtool] winget is not available. Install "App Installer" from Microsoft Store or Python from https://www.python.org/ and relaunch.
		pause
		exit /b 1
	)
	winget install --id Python.Python.3.12 -e --accept-source-agreements --accept-package-agreements
	if errorlevel 1 (
		echo [devtool] Python installation failed. If you are behind a proxy, set HTTP_PROXY/HTTPS_PROXY and run: netsh winhttp import proxy source=ie
		pause
		exit /b 1
	)
	echo [devtool] Python installed. Please close this window and relaunch devtool.cmd so PATH is refreshed.
	pause
	exit /b 0
)

%PY% "%DEVTOOL_DIR%server.py" --restart %*
endlocal
