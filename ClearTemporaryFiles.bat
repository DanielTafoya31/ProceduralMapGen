@echo off
cd /d "%~dp0"
rmdir /s /q .vs
rmdir /s /q Binaries
rmdir /s /q DerivedDataCache
rmdir /s /q Intermediate
rmdir /s /q Saved
del /q *.sln
cd "Plugins\Containers"
rmdir /s /q Binaries
rmdir /s /q Intermediate
