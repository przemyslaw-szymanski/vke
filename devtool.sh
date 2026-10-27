#!/usr/bin/env bash
# VKE DevTool launcher (Linux). Ensures python3, then (re)starts the server so code changes are always loaded.
set -e
DEVTOOL_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/tools/devtool"

if ! command -v python3 >/dev/null 2>&1; then
	echo "[devtool] python3 not found, installing..."
	. /etc/os-release 2>/dev/null || true
	case " ${ID} ${ID_LIKE} " in
		*" arch "*) sudo -E pacman -S --needed --noconfirm python ;;
		*" debian "*|*" ubuntu "*) sudo -E apt-get update && sudo -E apt-get install -y python3 ;;
		*) echo "[devtool] Unsupported distro '${ID}'. Install python3 manually."; exit 1 ;;
	esac
fi

exec python3 "${DEVTOOL_DIR}/server.py" --restart "$@"
