@echo off
SET mydir=%~dp0
SET curdir=%cd%

CD %mydir%

REM TODO: add progress bar, -q is quiet, if removing it the progress bar is in multiple lines
ECHO Downloading Boost
REM wget -q  https://dl.bintray.com/boostorg/release/1.68.0/source/boost_1_68_0.tar.gz
ECHO Unzipping Boost
REM tar xzvf boost_1_68_0.tar.gz >NUL
ECHO Installing Boost
CD boost_1_68_0

REM BOOST_ROOT was set by CMake, but the batch file doesn't have the variable, so set it again
SET BOOST_ROOT=%cd%\boost

REM bootstrap.bat >NUL
b2.exe --prefix=%cd%\installed --with-program_options link=static install toolset=msvc >NUL

CD %curdir%