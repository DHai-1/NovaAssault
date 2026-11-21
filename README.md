# Nova Assault — bản GameView "arcade shooter"

Thay thế 2 file trong project Haiku:

- `GameView.h`
- `GameView.cpp`

## Cách làm trong VM Haiku

```sh
cd ~/NovaAssault
cp GameView.h GameView.h.bak
cp GameView.cpp GameView.cpp.bak
# copy 2 file từ nova_patch vào đây, sau đó:
make
./objects.x86_64-cc13-release/NovaAssault
```

## Thay đổi chính

- Fix lỗi build: `#include <Roster.h>` (definition của `app_info` nằm ở đây, `<Application.h>` chỉ forward-declare).
- Dùng spritesheet CC0 có sẵn thay vì vẽ tay:
  - `ship.png`: 5 frame 16x24, vẽ ở kích thước 36x52 (thay vì phóng 120x180 bị vỡ hạt).
  - `enemy-small/medium/big.png`: 3 loại địch, hoạt hình theo frame.
  - `explosion.png`: 5 frame nổ khi bắn hạ địch / trúng tàu.
  - `laser-bolts.png`: laser player/enemy.
  - `power-up.png`: item P (tăng sát thương) / S (khiên).
- Nền: sao 3 lớp cuộn + vệt sáng + lưới phối cảnh + tinh vân.
- HUD kiểu arcade: thanh trên, SCORE/LIVES/WAVE/POWER, SHIELD ACTIVE/OFFLINE, GAME OVER.
- 18 địch chia 3 loại theo wave; địch bay quá đáy sẽ bị loại (fix bug wave không bao giờ sang).
- Có muzzle/exhaust, flash khi trúng đạn, delay giữa các wave.
