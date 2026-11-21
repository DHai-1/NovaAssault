# Nova Assault — bản v3 "xịn thật xịn"

Tiếng động, background mới, boss khổng lồ và item bắn hình quạt.

## File trong patch này

| File | Nội dung |
|---|---|
| `GameView.h` / `GameView.cpp` | Toàn bộ game: âm thanh, nền, boss, item quạt, hiệu ứng |
| `main.cpp` | signature `v3`, nhịp 20ms (~50fps) |
| `Makefile` | **thêm `game` vào LIBS** để dùng `BSimpleGameSound` |
| `run_nova.sh` / `fix_launcher.sh` | chạy game / sửa launcher |

## Cách áp dụng trong VM Haiku

```sh
cd ~/NovaAssault
git pull
make
./objects.x86_64-cc13-release/NovaAssault
```

Nếu bạn áp dụng bằng tay (không dùng git):

```sh
cd ~/NovaAssault
cp /path/to/nova_patch/GameView.h GameView.h
cp /path/to/nova_patch/GameView.cpp GameView.cpp
cp /path/to/nova_patch/main.cpp main.cpp
cp /path/to/nova_patch/Makefile Makefile
make
./objects.x86_64-cc13-release/NovaAssault
```

## Điều khiển

- `A` / `D` hoặc **di chuyển chuột**: lái tàu
- Bắn tự động, `P` tăng sức mạnh, `S` khiên, **`F` bắn quạt**
- `M`: bật/tắt tiếng
- `R`: chơi lại

## Điểm mới ở v3

### Có tiếng (không cần file nhạc)
- Dùng `BSimpleGameSound` của Haiku Game Kit, tạo PCM trực tiếp trong code.
- 8 hiệu ứng: bắn, trúng đich, nổ, nhặt item, tàu bị bắn, wave mới, cảnh báo boss, game over.
- Mỗi loại có 3 voice phát luân phiên nên bắn liên tục không bị "ngắt".
- Nếu VM chưa cấu hình audio, `InitCheck()` trả lỗi → game vẫn chạy im lặng, không crash.

### Background mới
- Tinh vân 4 lớp + mây CC0 trôi parallax.
- **Hành tinh có vành** lớn góc phải (với dải màu, bóng tối, viền sáng).
- **Mặt trăng** có hố nhỏ bên trái.
- Sao 3 lớp nhấp nháy theo thời gian + **sao băng** lướt qua.
- Lưới phối cảnh và vignette 2 bên.

### Boss to
- Xuất hiện ở **wave 5, 10, 15...** với banner `WARNING: BOSS`.
- Sprite `enemy-big.png` phóng to 4 lần (~256px), có lõi phát sáng, 2 thân pháo.
- Máu riêng `BOSS - OVERLORD` ngay dưới HUD.
- Di chuyển ngang, bắn **quạt 7 viên**, cứ 3 volley lại bắn **chùm 5 viên nhắm vào tàu**.
- Khi chết: 6 vụ nổ dây chuyền, rung màn hình, flash, cộng điểm lớn và **rơi item quạt chắc chắn**.

### Item bắn quạt (F)
- Rơi ngẫu nhiên 12% từ địch, và chắc chắn từ boss.
- Khi nhặt: bắn **5 viên dạng quạt** (7 viên nếu POWER x3) trong 12 giây.
- Đạn quạt có vệt sáng nghiêng theo hướng bay, dễ nhìn.

## Giữ nguyên
- Asset CC0 có sẵn, không copy assets game thương mại.
- Fix `app_info` (`#include <Roster.h>`), signature `v3`, mouse steering, game over panel, đạn tròn.
