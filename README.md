# Nova Assault 🚀

Game bắn tàu không gian viết bằng **C++ / BView** cho **Haiku**.  
Phiên bản **v3**: có tiếng, background vũ trụ mới, boss khổng lồ và item bắn hình quạt.

---

## 📥 Cài đặt lần đầu

**Tất cả lệnh trong phần này đều gõ trong Terminal của Haiku.**

```sh
cd /boot/home
git clone https://github.com/DHai-1/NovaAssault.git
cd NovaAssault
make
```

---

## ▶️ Chạy game

**Cách 1 — chạy trực tiếp file build:**

```sh
./objects.x86_64-cc13-release/NovaAssault
```

**Cách 2 — dùng script có sẵn:**

```sh
chmod +x run_nova.sh fix_launcher.sh
./run_nova.sh
```

**Muốn có icon game hiện lên màn hình / Deskbar:**

```sh
./fix_launcher.sh
```

---

## 🔄 Cập nhật khi mình push bản mới

```sh
cd /boot/home/NovaAssault
git pull
make
./objects.x86_64-cc13-release/NovaAssault
```

> Nếu `git pull` báo lỗi cũ `/boot/trash/NovaAssault`, dùng:
> ```sh
> git fetch origin
> git reset --hard origin/main
> make
> ```
> (chỉ dùng khi bạn **không có thay đổi local chưa commit**).

---

## 🎮 Điều khiển

| Phím / Thao tác | Chức năng |
|---|---|
| `A` / `D` | Di chuyển tàu trái / phải |
| **Di chuyển chuột** | Lái tàu theo chuột (không cần click) |
| Click chuột | Đổi vị trí tàu |
| Bắn | **Tự động**, không cần bấm |
| `M` | Bật / tắt tiếng (HUD hiện `SOUND ON/OFF`) |
| `R` | Chơi lại từ đầu |

---

## 🎁 Item rơi từ địch

| Chữ | Màu | Tác dụng |
|---|---|---|
| `P` | Vàng | Tăng POWER (tối đa `x3`) |
| `S` | Xanh ngọc | Bật khiên chắn đạn |
| `F` | **Tím** | **Bắn hình quạt trong 12 giây** |

- Nhặt `F` → HUD hiện **`FAN 12s`**.
- Đạn quạt: **5 viên** (hoặc **7 viên** nếu đang `POWER x3`), có vệt sáng nghiêng theo hướng bay.
- `F` rơi ngẫu nhiên ~12% từ địch, **rơi chắc chắn khi hạ boss**.

---

## 👾 Boss

- Xuất hiện ở **wave 5, 10, 15...** với banner `WARNING: BOSS`.
- Sprite phóng to 4 lần (~256px), có lõi phát sáng và 2 thân pháo.
- Thanh máu riêng `BOSS - OVERLORD` ngay dưới HUD.
- Bắn **quạt 7 viên**; cứ 3 volley bắn thêm **chùm 5 viên nhắm vào tàu**.
- Khi chết: 6 vụ nổ dây chuyền, rung màn hình, flash, cộng điểm lớn và rơi item `F`.

---

## ✨ Điểm mới ở v3

### 🔊 Có tiếng (không cần file nhạc)
- Dùng `BSimpleGameSound` (Haiku Game Kit), tạo PCM ngay trong code.
- 8 hiệu ứng: bắn, trúng địch, nổ, nhặt item, tàu bị bắn, wave mới, cảnh báo boss, game over.
- Mỗi loại 3 voice phát luân phiên → bắn liên tục không bị ngắt.
- `Makefile` đã thêm `game` vào `LIBS`.
- Nếu VM chưa có audio, game vẫn chạy im lặng, không crash.

### 🌌 Background mới
- Tinh vân 4 lớp + mây CC0 trôi parallax.
- **Hành tinh có vành** lớn góc phải (dải màu, bóng tối, viền sáng).
- **Mặt trăng** có hố nhỏ bên trái.
- Sao 3 lớp nhấp nháy + **sao băng** lướt qua + lưới phối cảnh.

### 💥 Hiệu ứng / HUD
- Đạn có vầng sáng tròn + lõi trắng (hết ô chữ nhật).
- Tàu có đuốc phản lực 3 lớp + bóng mờ khi lướt nhanh + vòng khiên.
- Nổ có vòng sốc + tia lửa; rung màn hình + flash khi bị bắn.
- Combo `COMBO xN`, banner `WAVE xx`, HUD arcade, icon tàu làm mạng.
- Màn `GAME OVER` có panel stats: FINAL SCORE / WAVE REACHED / BEST COMBO.
- Nhịp chạy 50fps (20ms/frame).

---

## 🔊 Không nghe thấy tiếng?

1. Trong **VMware → VM Settings → Sound Card**: tick **Connected** và **Connect at power on**.
2. **Tắt hẳn VM rồi bật lại** (Shut Down, không phải Restart).
3. **Mở lại game từ đầu** — nhấn `R` trong game không khởi tạo lại âm thanh.
4. Trong Haiku: mở **Deskbar → loa** kéo volume lên, hoặc **Preferences → Media**.
5. Nếu VMware đang "Specify host sound card", thử đổi sang **Use default host sound card**.
6. Trong game nhấn `M` xem có đang `SOUND OFF` không.

---

## 📁 Cấu trúc chính

| File | Vai trò |
|---|---|
| `main.cpp` | Khởi tạo ứng dụng, signature `application/x-vnd.dhai-novaassault-v3`, runner 20ms |
| `GameView.h` / `GameView.cpp` | Toàn bộ gameplay, âm thanh, nền, boss, item, hiệu ứng |
| `Makefile` | Build bằng `make` (có `-lgame` cho âm thanh) |
| `run_nova.sh` | Script chạy game |
| `fix_launcher.sh` | Tạo/sửa launcher để có icon trên Deskbar |

---

## 📄 Asset

Dùng asset **CC0** có sẵn trong `assets_cc0/space_shooter_pack/` (không copy asset bản quyền của game thương mại).  
Xem chi tiết trong `assets_cc0/space_shooter_pack/license.txt`.
