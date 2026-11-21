#!/bin/sh
# Xoa launcher cu (co the tro toi /boot/trash) va tao lai tu binary that.
set -e

cd /boot/home/NovaAssault || exit 1

# Xoa cac launcher/shortcut cu neu co
rm -f /boot/home/Desktop/NovaAssault \
      /boot/home/NovaAssault/NovaAssault \
      /boot/home/config/settings/LaunchBox/Applications/NovaAssault 2>/dev/null || true

# Tao launcher moi tro dung vao binary trong objects.*
BIN="/boot/home/NovaAssault/objects.x86_64-cc13-release/NovaAssault"
if [ ! -x "$BIN" ]; then
	echo "Khong tim thay binary: $BIN"
	echo "Chay 'make' truoc."
	exit 1
fi

ln -sf "$BIN" /boot/home/Desktop/NovaAssault

# Dang ky lai MIME/signature voi registrar
mimeset -f "$BIN"
killall registrar 2>/dev/null || true

echo "Xong. Chay bang lenh: ./objects.x86_64-cc13-release/NovaAssault"
