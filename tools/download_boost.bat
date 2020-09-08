@echo off
SET mydir=%~dp0
SET curdir=%cd%

CD %mydir%

REM TODO: add progress bar, -q is quiet, if removing it the progress bar is in multiple lines
ECHO Downloading Boost
curl -sL --output boost_1_68_0.tar.gz https://dl.bintray.com/boostorg/release/1.68.0/source/boost_1_68_0.tar.gz
ECHO Unzipping Boost
tar xzf boost_1_68_0.tar.gz >NUL
ECHO Installing Boost
CD boost_1_68_0

CALL bootstrap.bat >NUL
CALL b2.exe --prefix="%cd%\win_installed" --with-program_options --with-timer --with-chrono --with-thread --with-date-time --with-atomic link=static install toolset=msvc >NUL

CD %curdir%
