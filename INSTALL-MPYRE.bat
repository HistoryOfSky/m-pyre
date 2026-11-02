@echo off
title M-PYRE installer
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0build-mpyre.ps1"
pause
