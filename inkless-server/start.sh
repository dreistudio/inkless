#!/usr/bin/env bash
set -e
cd "$(dirname "$0")"
exec /usr/bin/python3 -m uvicorn server:app --host 0.0.0.0 --port 8100
