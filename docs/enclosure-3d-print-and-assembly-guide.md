# Hộp in 3D cho AirPresenter S3

Model là dạng tham số (OpenSCAD), nằm trong [enclosure/](../enclosure/). File STL ở `enclosure/stl/`, ảnh xem trước ở `enclosure/preview/`. Đấu dây xem ở [hardware-wiring-and-firmware-usage-guide.md](hardware-wiring-and-firmware-usage-guide.md).

## 1. Hình dạng

- Kích thước ngoài **122 × 44 × 21 mm**, cạnh vát 45°, nắp nút nhô thêm khoảng 4.5 mm.
- Gồm các phần in:
  - vỏ dưới
  - vỏ trên
  - 12 nắp nút có ký hiệu
  - miếng in thử khớp
- Hai nửa vỏ ghép bằng gờ định vị và **4 ốc M3 × 10 tự ren**, vặn từ đáy. Muốn dùng ốc dài khác thì đổi `screw_len` trong `enclosure-parameters.scad`; lỗ âm tự sâu thêm hoặc nông lại để ốc luôn ăn ren 5 mm.

```
            ĐẦU (chĩa vào màn hình)
   ┌──────[USB-C ESP32: nạp code]─────┐
   │  ESP32-S3          GY-521         │
   │ (ốc)                       (ốc)   │
   │  ┌─────────────────────────────┐  │
   │  │     PIN 51 × 34 × 8 mm      │  │
   │  └─────────────────────────────┘  │
 ══╡công tắc                           │   ← cần gạt nguồn ở cạnh trái
   │ (ốc)       TP4056          (ốc)   │
   └──────────[USB-C sạc pin]──────────┘
            ĐUÔI
```

## 2. Cách giữ từng linh kiện (không cần keo)

| Linh kiện | Giữ bằng |
|---|---|
| Nút 6×6×8 | Ấn **từ mặt trên** xuống hốc, tới khi tì lên gờ đỡ. 4 chân xuyên qua 4 lỗ của gờ. Khi nhấn, lực ép nút xuống gờ nên nút không bị đẩy tụt vào trong hộp. Hàn dây vào chân (hoặc bẻ chân) dưới gờ là nút không rút ra được. |
| TP4056 | Thả thẳng xuống, mặt linh kiện hướng lên: cổng USB-C lọt vào **rãnh chữ U** ở thành hộp, mạch tì lên gờ đỡ cao 1.5 mm, có gờ dẫn hai bên và gờ chặn phía sau (đỡ lực khi cắm cáp). Đóng nắp thì **lưỡi khóa** trên vỏ trên ép cổng USB xuống, cột trên nắp chặn đầu còn lại. |
| ESP32-S3 | Lắp **úp**: mặt có chân/pad (mặt dưới) hướng lên, để hàn dây được. Cổng USB-C khi đó nằm dưới mạch, tì lên một bệ và lọt vào rãnh ở mũi; lưỡi khóa trên nắp ép xuống phần cổng nhô vào trong thành. Mạch được **đỡ đủ 4 cạnh**:
- Cạnh có cổng USB: bệ dưới cổng.
- Cạnh đối diện: một gờ.
- Hai cạnh dài: 2 thanh đỡ bậc thang. Bậc thấp nằm dưới mép mạch, bậc cao là tường chắn bên ngoài.

Các gờ đều chỉ chạm dải 0.6 mm sát mép (`pcb_edge_rest`), nên linh kiện đang chúc xuống nằm lọt ở giữa. Xem [ảnh phần đỡ](../enclosure/preview/esp32-cradle.png). Mặt trên mạch cách các nút phía trên khoảng 4 mm: đủ cho mối hàn và dây, nhưng **không đủ cho hàng chân header**. Nếu board đã hàn sẵn header thì tháo ra, hoặc cắt chân còn ≤ 1.5 mm rồi hàn dây vào. Xem [mặt cắt](../enclosure/preview/esp32-mount-section.png). |
| GY-521 | Thả vào hốc có 4 góc chặn hình chữ L, kê trên 2 gờ. Cột trên nắp cách chip 0.4 mm nên module không nhấc lên được. |
| Công tắc gạt | Thả từ trên xuống: cần gạt vào khe ở thành hộp, thân vào hốc kín 4 phía (chừa chỗ cho 3 chân). Lưỡi khóa trên nắp đóng phần khe phía trên cần gạt. |
| Pin | Gờ 4 phía giữ ngang. 2 cột trên nắp, cách mặt pin 1.2 mm, giữ không cho pin nhấc lên. |

Ảnh mặt cắt một nút: [button-socket-section.png](../enclosure/preview/button-socket-section.png). Nút lắp sao cho **2 hàng chân quay về hai bên trái/phải của remote**, vì chiều 7.6 mm của lỗ nằm theo bề ngang remote.

Khi mở nắp, mọi thứ chỉ cần nhấc thẳng lên là ra, rất tiện để sửa. Băng keo xốp chỉ còn là tùy chọn, nếu muốn hết hẳn tiếng lục cục.

## 3. ĐO LINH KIỆN TRƯỚC KHI IN (quan trọng)

Mỗi shop bán một kích thước khác nhau, lệch 1–3 mm:
- ESP32-S3 SuperMini có listing ghi 22.52 × 18, có listing ghi 23.2 × 18.2 hoặc 25 × 18.
- TP4056 Type-C từ 25 đến 29 mm.

Model đang dùng số trong bảng dưới. **Dùng thước kẹp đo đồ thật**, sửa khối `MEASURE` ở đầu `enclosure/enclosure-parameters.scad` (hoặc gửi số cho Claude cập nhật giúp).

| Linh kiện | Đo | Tham số | Đang dùng |
|---|---|---|---|
| ESP32-S3 SuperMini | rộng · dài mạch (không tính cổng USB) · dày mạch · cổng USB-C rộng · cao · **nhô ra khỏi mép mạch** · mối hàn/chân cao nhất ở mặt có chân (mặt hướng lên) · `1` = lắp úp | `esp_board` | 18 · 22.6 · 1.2 · 8.94 · 3.26 · 0.6 · 1.5 · 1 |
| TP4056 | như trên, nhưng số thứ 7 là linh kiện cao nhất ở mặt linh kiện · `0` = lắp ngửa | `tp_board` | 17.3 · 28 · 1.2 · 8.94 · 3.26 · 0.6 · 1.8 · 0 |
| GY-521 | rộng · dài · dày mạch | `gy_board` | 16.4 · 21.2 · 1.6 |
| Pin | rộng · dài · dày | `bat_size` | 34 · 51 · 8 |
| Công tắc gạt | thân: cao (không tính cần) · dài · rộng | `ss_body` | 3.6 · 8.8 · 3.9 |
| | cần gạt: dài · rộng · dày | `ss_lever` | 4 · 2 · 1.6 |
| | hành trình | `ss_travel` | 2 |
| Nút 6×6 | thân · tổng cao (đáy thân → đỉnh trục) · Ø trục · khoảng cách chân | `tact_body`, `tact_total_h`, `tact_act_d`, `tact_pin` | 6 · 8 · 3.5 · 6.5×4.5 |

Hai số dễ sai nhất:
- **Chiều dài mạch.** Gờ chặn phía sau đặt đúng ở cuối mạch. Mạch ngắn hơn số trong model thì cắm cáp sẽ đẩy mạch lùi lại.
- **Độ nhô của cổng USB-C.** Số này quyết định cáp có cắm vào hết được không.

Dung sai đang dùng, mỗi bên:

| Chỗ | Hở |
|---|---|
| mép mạch với gờ dẫn, gờ chặn | 0.3 mm |
| cổng USB-C với rãnh | 0.2 mm |
| lưỡi khóa với mặt cổng USB | 0.1 mm |
| thân công tắc gạt | 0.25 mm |
| thân nút trong hốc | 0.2 mm |
| hai phần in với nhau | 0.2 mm |

Lỗ hốc nút trên mặt là 7.6 × 6.4 mm, đủ cho cả chân nút lọt qua.

## 4. In thử khớp trước (khoảng 1 giờ)

`stl/fit-test-coupon.stl` gồm các lát cắt thật của hộp:
- **A:** đuôi vỏ dưới, có hốc TP4056, rãnh cổng sạc và hốc công tắc.
- **B:** đầu vỏ dưới, có hốc ESP32 và GY-521, rãnh cổng nạp.
- **C:** 3 hốc nút.
- **D:** đuôi vỏ trên, có lưỡi khóa và các cột, úp vừa lên A.
- **E:** 2 nắp nút.

Cách thử:
1. In xong, đặt TP4056, công tắc, ESP32, nút và nắp vào. Úp D lên A để thử lưỡi khóa.
2. Cáp USB-C phải cắm hết vào cổng.
3. Board không được lỏng lắc, nút phải bấm "tách" rõ.
4. Nếu chặt quá hoặc lỏng quá thì chỉnh dung sai hoặc số đo ở mục 3 rồi in lại miếng thử. Khi vừa hết mới in cả hộp.

## 5. In

- PLA hoặc PETG (PETG dai hơn), lớp 0.2 mm, 3 vòng tường, infill 20%.
- **Tắt support (Enable support: Off)** cho cả 2 vỏ. Model tự in được, đã thử cắt lớp bằng Bambu Studio và ra 0 support:
  - Gờ ghép của vỏ dưới có **gờ đỡ vát 45°** nối vào thành. Trước đây gờ lơ lửng nên slicer mọc cây support dọc 2 thành dài.
  - Hốc đầu vít dưới đáy có **2 lớp bắc cầu hy sinh** (một rãnh, rồi một ô vuông, cùng rộng bằng lỗ vít). Vít xuyên qua bình thường.
  - Lỗ cắm cáp USB-C có mái vát 45° ở 2 góc trên.
  - Trần 12 hốc nút là bắc cầu ngắn 6.4 mm.
  - Nếu vẫn muốn bật support: vỏ dưới dùng **Tree (auto)** (0 support). Vỏ trên dùng **Normal (auto) + "Don't support bridges"** (0 support). Kiểu Tree luôn đỡ mọi đoạn bắc cầu nên sẽ mọc support trong 12 hốc nút.
- Nhựa cần (Bambu A1, PLA, tắt support): vỏ dưới ~21.6 g / 35 phút, vỏ trên ~20.8 g / 45 phút, nắp nút ~3.3 g / 11 phút.
- **File STEP** (mỗi phần một file, đơn vị mm) nằm ở `enclosure/step/`:
  - `top-shell.step`: vỏ trên
  - `bottom-shell.step`: vỏ dưới
  - `button-caps.step`: 12 nắp nút (12 khối riêng trong cùng một file)

  Bambu Studio mở thẳng được. Hướng đặt giống STL. Mặt cong (trụ ốc, cạnh vát) là đa giác chia mịn, in ra không thấy khác.
- STL đã đặt sẵn hướng in:
  - `bottom-shell.stl`: đáy úp xuống bàn.
  - `top-shell.stl`: mặt có nút úp xuống bàn.
  - `button-caps.stl`: mặt có ký hiệu hướng lên.
  - `fit-test-coupon.stl`: đặt sẵn để in một lượt.

## 6. Vật tư thêm

| Thứ | Ghi chú |
|---|---|
| 4 ốc M3 × 10 tự ren, đầu tròn (đường kính đầu ≤ 6 mm) | vặn thẳng vào nhựa; lỗ âm dưới đáy sâu 6 mm |
| Keo 502 | 1 giọt cho mỗi nắp nút |
| Dây điện mảnh AWG 28–30 | khoảng 14 sợi |
| (tùy chọn) băng keo xốp mỏng | dán đáy pin, chống lục cục |

## 7. Lắp ráp

1. **Nút:**
   - 4 lỗ chân trong mỗi hốc được phủ một **màng mỏng 0.2 mm** (để trần hốc in bắc cầu được). Chọc thủng trước bằng kim hoặc đầu mỏ hàn nóng, hoặc ấn mạnh cho chân nút nhấn tự xuyên qua. (Đây là nút nhấn 6×6, không liên quan công tắc gạt nguồn.)
   - Ấn 12 nút từ mặt trên của vỏ trên vào hốc, chân xuyên qua gờ đỡ.
   - Lật vỏ lại, hàn một dây GND chung qua 1 chân của mọi nút. Chân **chéo** với nó hàn dây tín hiệu riêng, ghi tên nút lên dây.
2. **Vỏ dưới:**
   - Thả TP4056 xuống, cổng USB-C vào rãnh ở đuôi.
   - Thả công tắc gạt, cần vào khe bên trái.
   - Đặt pin vào khoang, đầu có dây quay về phía TP4056.
   - Hàn dây vào ESP32 trước. Sau đó **lật úp** board, mặt có chân hướng lên, rồi thả vào góc trước-trái: cổng USB-C (lúc này nằm dưới) tì lên bệ và lọt vào rãnh ở mũi.
   - Đặt GY-521 vào hốc trước-phải, chip hướng lên, mũi tên X chỉ về đầu remote. Lắp hướng khác cũng được: đóng hộp xong thì dạy hướng mũi một lần (giữ FN 2 s, thả, chĩa mũi lên trần, bấm AIR; xem [hướng dẫn](hardware-wiring-and-firmware-usage-guide.md) mục 4, bước 7).
3. Đấu dây theo [hướng dẫn](hardware-wiring-and-firmware-usage-guide.md): diode 1N5819, cầu chia áp 100k/100k. Dồn dây vào khe giữa các hốc nút và các cột.
4. **Nạp firmware và thử hết các nút khi hộp còn mở**, xem log bằng `pio device monitor`.
5. Úp vỏ trên xuống: gờ vỏ dưới lọt vào trong, 2 lưỡi khóa lọt vào 2 rãnh USB, lưỡi nhỏ lọt vào khe công tắc. Vặn 4 ốc từ đáy.
6. Ấn nắp nút lên trục. Khít rồi thì nhỏ 1 giọt keo 502 vào lỗ nắp rồi ấn lại.

## 8. Xem 3D trong VS Code

Trang xem nằm trong `enclosure/viewer/`. Có 4 chế độ:
- **Đóng hộp**
- **Mở nắp**: vỏ trên lật ngửa đặt cạnh vỏ dưới
- **Tách lớp**
- **Cắt đôi**: có thanh kéo để dời vị trí mặt cắt

Trang còn cho bật/tắt từng chi tiết, làm vỏ trong suốt, và bấm vào chi tiết để xem tên, kích thước, cách giữ.

1. Chạy server:
   ```bash
   python3 -m http.server 8765 --bind 127.0.0.1 --directory enclosure/viewer
   ```
2. Trong VS Code: **Ctrl+Shift+P** → `Simple Browser: Show` → `http://127.0.0.1:8765`.

## 9. Sửa model

- Cài OpenSCAD (bản snapshot mới render nhanh hơn nhiều).
- Mở `enclosure/air-presenter-enclosure.scad` và đổi `part` để xem từng phần.
- Sửa số xong thì render `fit_check_bottom`, `fit_check_top`, `fit_check_shells`. **Thể tích phải bằng 0**: chỉ được phép có mặt tiếp xúc (mạch tì gờ, nút tì gờ đỡ), không có chỗ đè lên nhau.
- Xuất lại:
  ```bash
  cd enclosure
  openscad --backend=manifold --export-format binstl -D 'part="top_print"' -o stl/top-shell.stl air-presenter-enclosure.scad   # tương tự cho bottom_print, caps, fit_test
  # kiểm tra va chạm: fit_check_bottom / fit_check_shells phải ra thể tích 0; fit_check_top ra ~2 mm3 = chân nút nhấn xuyên màng 0.2 mm
  viewer/export-viewer-models.sh   # cập nhật trang xem 3D
  # công cụ cài cố định trên máy này: ~/.local/bin/openscad (OpenSCAD 2026.10.15) và ~/.local/opt/occ-venv (cadquery-ocp + vtk)
  ~/.local/opt/occ-venv/bin/python convert-stl-to-step.py    # stl/*.stl (dạng nhị phân) -> step/*.step, kèm kiểm tra khối đặc và thể tích
  ```
