echo off
cls

echo Building for Visual Studio 2015...
"%~dp0premake5" vs2015 --file="%~dp0premake5.lua"