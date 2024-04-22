@echo off
pushd %~dp0
set PATH=%CD%;%PATH%
set CPATH=%CD%e;%CPATH%
set LIBRARY_PATH=%CD%;%LIBRARY_PATH%
popd
