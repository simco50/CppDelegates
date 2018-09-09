echo off
cls

echo Building for Visual Studio 2017...
"%~dp0premake5" vs2017 --file="%~dp0premake5.lua"