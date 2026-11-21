@echo off
rem Server lokal gratis (Python 3). Buka http://localhost:8080
cd /d "%~dp0\..\web"
python -m http.server 8080
