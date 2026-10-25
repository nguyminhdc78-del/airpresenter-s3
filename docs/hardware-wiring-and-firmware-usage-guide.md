# AirPresenter S3: Đấu dây, nạp firmware, sử dụng

Remote thuyết trình + chuột bay dùng ESP32-S3 SuperMini, kết nối với PC qua Bluetooth LE (HID).

## 1. Đấu dây (ESP32-S3 SuperMini)

Cả 12 nút đều là nút nhấn 6x6 **gắn trên remote**, kể cả FN. Mỗi nút có 1 chân nối vào GPIO, chân còn lại nối về GND chung. Không cần điện trở vì firmware bật pull-up nội.

Nút 6x6 có 4 chân, 2 chân cùng một bên luôn nối thông với nhau. Cứ dùng **2 chân chéo nhau**: một chân vào GPIO, chân kia vào GND.

| Chân ESP32 | Nút trên remote |
|---|---|
| GPIO2 | AIR (giữ để di chuột theo tay) |
| GPIO3 | CLICK |
| GPIO4 | NEXT |
| GPIO5 | PREV |
| GPIO6 | PLAY |
| GPIO7 | VOL+ |
| GPIO8 | VOL- |
| **GPIO9** | **FN** (nút phụ trên remote, xem mục 2) |
| GPIO10 | UP |
| GPIO11 | DOWN |
| GPIO44 (chân in chữ **RX**) | LEFT |
| GPIO43 (chân in chữ **TX**) | RIGHT |

| Linh kiện | Nối |
|---|---|
| GY-521 (MPU6050) | VCC→3V3, GND→GND, SDA→**GPIO12**, SCL→**GPIO13** (AD0 để trống → địa chỉ 0x68) |
| hoặc module MPU6500 (bo tím) | VCC→3V3, GND→GND, SDA→**GPIO12**, SCL→**GPIO13**, AD0→GND. Bỏ trống EDA/ECL/INT/FSYNC. Nếu log báo không thấy cảm biến thì nối thêm NCS→3V3 |
| LED trạng thái | dùng LED RGB có sẵn trên board (GPIO48) |
| Đo pin | OUT+ (sau công tắc, trước diode) → 100k → **GPIO1** → 100k → GND; thêm tụ 100 nF từ GPIO1 xuống GND |

```
LiPo+ ─ B+ [TP4056] OUT+ ─ công tắc ─┬─►|─ 5V (SuperMini)     ►| = Schottky 1N5819/SS14
LiPo- ─ B-          OUT- ─────────────┼──── GND                     (vạch trên diode quay về phía 5V)
                                      100k
                                      ├──── GPIO1  ══ 100nF ══ GND
                                      100k
                                      └──── GND
```

Quy tắc an toàn:
- **Không** nối pin vào chân 3V3.
- **Không** nối pin thẳng vào 5V mà bỏ diode. Đây là lỗi backfeed đã có người gặp trên SuperMini.
- Tắt công tắc pin khi cắm USB nạp code.
- Đừng giữ nút RIGHT (GPIO43) lúc cấp điện. Lúc khởi động, GPIO43 đang là chân TX của UART.
- Board của bạn có pad pin + mạch sạc sẵn? Khi đó dùng pad đó thay cho TP4056 + diode, và vẫn nối cầu chia áp từ BAT+ vào GPIO1.

Board khác (S3-Zero, Lolin S3 Mini…): sửa chân trong `firmware/src/remote-user-config.h` và `firmware/src/keymap-presenter-default.cpp`. Nút nào muốn dùng để đánh thức máy thì phải nằm trên GPIO 1–21.

## 2. Chức năng nút (mặc định)

**FN là nút nằm trên remote (GPIO9), không phải phím Fn của máy tính.** Nó giống phím Shift: tự nó không gửi gì lên PC, chỉ đổi chức năng của nút bấm kế tiếp. PC không bao giờ biết có nút FN. Cách dùng:
- **Bấm giữ:** giữ FN rồi nhấn nút kia.
- **Bấm 1 tay:** nhấn nhả FN (LED tím sáng), rồi trong 3 giây nhấn nút kia. Nhấn FN lần nữa để hủy.
- **Giữ riêng FN 2 giây rồi thả** (LED xanh lơ): dạy hướng mũi remote cho chế độ AIR (mục 4, bước 7).
- **Giữ riêng FN 10 giây:** xóa ghép đôi, dùng khi chuyển sang máy khác (LED trắng nháy 3 lần).

| Nút | Nhấn | FN + nút |
|---|---|---|
| AIR | giữ = di chuột theo tay | Ctrl+L (laser pointer của PowerPoint) |
| CLICK | click trái (giữ = kéo thả) | click phải |
| NEXT | slide sau (PageDown) | F5: bắt đầu trình chiếu |
| PREV | slide trước (PageUp) | Esc: thoát trình chiếu |
| PLAY | Play/Pause | Mute |
| VOL+ / VOL- | tăng / giảm âm lượng (giữ = lặp) | cuộn lên / xuống (giữ = lặp) |
| UP/DOWN/LEFT/RIGHT | di chuột từng chút (giữ = nhanh dần) | UP: B (màn hình đen) · DOWN: Alt+Tab · LEFT/RIGHT: bài trước/sau |
| FN (GPIO9) | đổi chức năng nút kế tiếp; giữ riêng 2 s rồi thả = dạy hướng mũi; giữ riêng 10 s = xóa ghép đôi | |
| BOOT (trên board ESP32) | giữ 10 s = xóa ghép đôi (giống giữ FN, dùng khi hộp đang mở) | |

Đổi chức năng nút: sửa 1 dòng trong `keymap-presenter-default.cpp`. Mã phím có sẵn trong `hid-report-descriptor-and-usages.h`.
- Dùng Mac: đổi `kModAlt` thành `kModGui` để có Cmd+Tab.
- Google Slides: phím bắt đầu trình chiếu là Ctrl+F5, không phải F5.

## 3. Build & nạp firmware

1. Cài VS Code + extension PlatformIO, hoặc cài `pio` CLI.
2. Mở thư mục `firmware/`.
3. Chạy các lệnh:
   ```bash
   pio run                 # build
   pio run -t upload       # nạp qua cổng USB-C của board
   pio device monitor      # xem log (115200)
   pio test -e native      # unit test logic chuột bay trên PC (phải có -e native)
   ```
4. Nếu nạp không được: giữ BOOT, nhấn RESET, thả BOOT, rồi nạp lại. Nạp xong nhấn RESET.

## 4. Ghép đôi & dùng

1. Bật nguồn. LED **vàng** nghĩa là đang hiệu chuẩn gyro: **để remote nằm yên** khoảng 1 giây. Nếu bị rung, firmware tự đo lại, tối đa 4 lần. Khi thức dậy từ ngủ sâu thì không cần bước này: remote dùng lại giá trị đã lưu trước khi ngủ.
2. LED **xanh dương nháy** nghĩa là đang chờ kết nối.
   - Windows: Settings → Bluetooth & devices → Add device → Bluetooth → **AirPresenter S3**.
   - macOS / Android: vào phần cài đặt Bluetooth.
3. LED **xanh lá** 1.5 s nghĩa là đã kết nối, sau đó LED tắt để tiết kiệm pin. Những lần sau remote tự kết nối lại.
4. LED **đỏ nháy ngắn** nghĩa là pin yếu (≤15%). Phần trăm pin cũng hiện trong cài đặt Bluetooth của Windows/Android.
5. Không dùng 30 phút (hoặc 3 phút mà chưa kết nối) thì remote tự ngủ sâu. Nhấn một nút bất kỳ trên GPIO 2–11 để đánh thức; remote tự kết nối lại sau khoảng 1–3 s. Lần nhấn dùng để đánh thức sẽ không được gửi lên PC.
6. Chuyển sang máy khác: giữ riêng nút FN 10 s (LED trắng nháy 3 lần), **thả ra**, rồi ghép đôi trên máy mới. Nên vào máy cũ "Remove device"; nếu không, máy cũ sẽ liên tục thử kết nối lại.
7. **Dạy hướng mũi** (làm 1 lần sau khi lắp hoặc lắp lại GY-521; GY-521 nằm ngửa, úp, xoay kiểu nào cũng được):
   - Cầm remote bình thường (mặt trên hướng lên), giữ riêng FN khoảng 2 s tới khi LED **xanh lơ** thì thả ra ngay. LED chuyển sang xanh lơ nháy nhanh. **Đừng giữ tới 10 s**: giữ tới 10 s là xóa ghép đôi, khi đó laptop sẽ kết nối rồi ngắt liên tục cho tới khi bạn xóa remote khỏi Bluetooth của laptop và ghép lại.
   - Chĩa **mũi remote thẳng lên trần nhà**, giữ yên một nhịp rồi **bấm AIR**.
   - LED **xanh lá nháy 2 lần** là đã lưu. **Đỏ nháy 2 lần** là không đổi gì: lúc bấm AIR mũi chưa chĩa thẳng lên, hoặc quá 15 s chưa bấm. Làm lại từ đầu.
   - Lỡ vào chế độ này mà không muốn dạy thì cứ dùng bình thường: bấm AIR khi cầm ngang sẽ hủy, không lưu gì.
   - Hướng mũi lưu trong flash: tắt nguồn hay nạp lại firmware cũng không mất. Trái/phải luôn đúng nhờ trọng lực; bước này để lên/xuống đúng.

Hệ điều hành dùng được: **Windows 10/11**, macOS, Linux, Android (iPad/iPhone thì dùng phím được, còn chuột phải bật AssistiveTouch). Remote là thiết bị Bluetooth HID chuẩn nên không cần cài driver. Cần máy có Bluetooth 4.0 trở lên; Windows 7 không hỗ trợ bàn phím/chuột Bluetooth LE. PC bàn không có Bluetooth thì cắm thêm USB Bluetooth.

## 5. Tinh chỉnh (`remote-user-config.h`)

| Hiện tượng | Chỉnh |
|---|---|
| Lên/xuống khó điều khiển, vặn cổ tay mà chuột chạy dọc, gật lên mà chuột đi xuống | dạy lại hướng mũi (mục 4, bước 7), không cần sửa code |
| Hướng mũi mặc định khi chưa dạy | `kAirForwardAxis` (0=X, 1=Y, 2=Z) + `kAirForwardSign` (±1): mũi tên trên GY-521 chỉ về đầu remote. Mặc định +X |
| Rê chậm vẫn rung | tăng `kAirSlowSmoothingSec` (mặc định 0.06 s), áp dụng khi tay xoay dưới `kAirTierSlowDps` = 4°/s |
| Vung nhanh thấy trễ | giảm `kAirSmoothingSec` (mặc định 0.008 s), áp dụng khi tay xoay trên `kAirTierFastDps` = 60°/s; ở giữa thì pha trộn dần |
| Bấm CLICK làm con trỏ giật | tăng `kClickSettleMs` (mặc định 100 ms: trong lúc giữ AIR, con trỏ đứng yên ngần ấy sau mỗi lần bấm/nhả chuột) |
| Muốn mượt nhất (133 gói/giây) | `kBleFastInterval = true`: xin chu kỳ Bluetooth 7.5 ms (máy không chịu thì tự lùi về 15 ms). Mặc định `false` (15 ms) vì laptop dùng chip QCA9377 đã treo cứng 1 lần sau khi bật 7.5 ms |
| Chậm hoặc nhanh quá | `kAirSensitivity` (mặc định 6) |
| Muốn vung nhẹ đã chạy xa | giảm `kAirAccelKneeDps` (mặc định 40) |
| Con trỏ rung khi giữ yên | tăng `kAirDeadzoneDps` (1.5 → 2.5) |
| D-pad chậm hoặc nhanh | `kNudgeMinCountsPerSec` / `kNudgeMaxCountsPerSec` |
| Ngủ sớm quá | `kIdleSleepTimeoutMs` |

Trên Windows nên tắt "Enhance pointer precision" (Mouse settings) rồi mới chỉnh độ nhạy.

## 6. Xử lý sự cố

- **Log báo `Wire.cpp ... Error 263` / `Error -1`, con trỏ AIR bị kéo lệch hoặc remote treo:** dây I2C tới GY-521 tiếp xúc chập chờn. Hay gặp nhất với dây Dupont: lỗi dồn dập lúc vung tay. Cách xử lý: bóp chặt đầu cái, dây dưới 10 cm, cố định dây; khi lắp hộp thì hàn thẳng. Firmware đã tự bỏ mẫu đọc rác và khởi động lại bus, nhưng không thay được mối nối tốt.
- **Phần mềm khác chiếm cổng USB của remote** (ví dụ một dịch vụ nền tự dò mọi thiết bị Espressif VID 303A): remote mất log, và từng bị khởi động lại đúng lúc phần mềm đó bắt đầu gửi dữ liệu. Cho phần mềm đó bỏ qua số serial USB của remote (xem bằng `pio device list`), hoặc tắt nó khi cắm remote.
- **Độ mượt và Bluetooth:** remote xin máy tính chu kỳ kết nối 7.5 ms (133 gói/giây). Máy không chịu thì sau 3 s tự lùi về 15 ms; log in `[ble] link: interval ...` cho biết máy đã chọn chu kỳ nào. Remote nhớ tối đa 4 máy đã ghép đôi.
- **Laptop kết nối rồi ngắt liên tục** (log báo `[ble] the PC keeps dropping the link before encryption`): remote đã xóa ghép đôi (giữ FN hoặc BOOT 10 s) nhưng laptop vẫn giữ khóa cũ. Xóa "AirPresenter S3" khỏi danh sách Bluetooth của laptop rồi ghép đôi lại.
- **Không nạp được, esptool báo `Write timeout`:** đưa board vào chế độ nạp bằng tay: giữ BOOT, nhấn rồi nhả RST, sau đó nhả BOOT. Nạp với `--before no_reset`, nạp xong nhấn RST.
- **Log `[power] last reset: ...`:** `firmware crash` hoặc `watchdog` nghĩa là firmware bị lỗi, hãy lưu lại log. `brownout` nghĩa là nguồn bị sụt áp: kiểm tra dây pin, công tắc gạt và diode.
- **Lên/xuống hoặc xiên không theo ý:** dạy lại hướng mũi (mục 4, bước 7). Muốn biết chắc thì mở log (`pio device monitor`): mỗi lần thả AIR in ra một dòng `[air] ... turned X.. Y.. Z.. deg -> horiz .. vert .. deg`. Gật lên xuống mà `vert` gần 0 nghĩa là trục mũi sai. Nếu `turned` cũng gần 0 thì tay đang trượt chứ không xoay: gyro chỉ đo được chuyển động xoay, nên phải nghiêng remote.
- **Không thấy remote trong danh sách Bluetooth:** nó vẫn đang kết nối/bond với máy khác. Giữ BOOT (hoặc riêng FN) 10 s.
- **Đã kết nối nhưng nút không có tác dụng** (thường xảy ra sau khi sửa report descriptor): Windows còn giữ descriptor cũ. Remove device rồi ghép đôi lại.
- **Log báo `[gyro] absent`:** kiểm tra dây SDA/SCL, VCC. Lúc này remote vẫn chạy được bằng D-pad.
- **LED sai màu** (đỏ/xanh lá bị đảo): board dùng LED thứ tự RGB. Chỉ ảnh hưởng màu hiển thị, không ảnh hưởng chức năng.
- **Nhấn nút không đánh thức được:** chỉ nút trên GPIO 1–21 đánh thức được (LEFT/RIGHT ở 43/44 và BOOT thì không).
