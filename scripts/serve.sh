#!/usr/bin/env bash
# Server lokal gratis (Python 3). Buka http://localhost:8080
cd "$(dirname "$0")/../web" && python3 -m http.server 8080
