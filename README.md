# Nova Assault — bản "xịn" v2 (hiệu ứng nâng cấp)

## File

| File | Dùng để |
|---|---|
| `GameView.h` / `GameView.cpp` | **v2** — parallax, particle, shake, combo, banner, HUD mới |
| `main.cpp` | signature `v3` + nhịp 20ms (~50fps) |
| `run_nova.sh` / `fix_launcher.sh` | chạy game / sửa launcher |

## Cách áp dụng trong VM Haiku

```sh
cd ~/NovaAssault
cp /path/to/nova_patch/GameView.h ./GameView.h
cp /path/to/nova_patch/GameView.cpp ./GameView.cpp
cp /path/to/nova_patch/main.cpp ./main.cpp
make
./objects.x86_64-cc13-release/NovaAssault
```

## Điểm mới ở v2

### Hình ảnh
- Nền gradient tự nhiên (-band) thay vì nền đen phẳng.
- Layers mây/tinh vân CC0 trôi ngang + 3 lớp sao parallax có vệt đuôi theo tốc độ + lưới phối cảnh + vignette 2 bên.
- Tàu: 5 frame sprite, **đuốc phản lực 3 lớp**, **bóng mờ (afterimage)** khi di chuyển nhanh, khiên hình vòng tròn nhấp nhô.
- Địch: 3 loại sprite, hiệu ứng **xuất hiện dần**, đèn flash khi trúng đạn, thanh máu riêng cho boss.
- Đạn laser: có **viền glow**, tia lửa bắn ra liên tục.
- Item P/S: **pulse + vòng sáng** riêng theo màu.
- Nổ: sprite 5 frame + **vòng sốc** loe dần + **tia lửa particle**.

### Hiệu ứng / cảm giác chơi
- **Rung màn hình** khi bắn hạ địch/trúng tàu (`PushState`/`SetOrigin`).
- **Flash trắng** khi tàu bị bắn.
- Hệ **combo**: `COMBO xN` + cộng điểm tăng dần, tự reset sau 2.5s.
- **Banner `WAVE xx`** mỗi wave mới (fade in/out).
- HUD arcade: tiêu đề có shadow+cyan, điểm số, **icon tàu làm biểu tượng mạng**, thanh SHIELD ACTIVE/OFFLINE, thanh máu boss.
- Nhịp chạy 50fps (trước 33fps).

## Giữ nguyên
- Asset CC0 có sẵn trong repo (không copy asset bản quyền của game thương mại).
- Fix `app_info` (`#include <Roster.h>`) và signature app `v3` (hết lỗi launcher `/boot/trash`).
