// What each STL piece is, how it is drawn and what the info card says about it.
// Keep the numbers in step with enclosure-parameters.scad.

export const CASE_SIZE = { length: 122, width: 44, height: 21 };

export const PIECES = [
  { id: 'top_shell', name: 'Vỏ trên', layer: 'top', shell: true, color: 0xdde2e8, facts: [
    '12 hốc nút: lỗ 7.6 × 6.4 mm trên mặt, gờ đỡ dày 1.2 mm có 4 lỗ chân',
    '2 lưỡi khóa: ép cổng USB-C của TP4056 và ESP32 vào rãnh',
    'Lưỡi khóa cần gạt, các cột chặn pin / board / công tắc',
    '4 trụ ốc, lỗ mồi Ø2.5 cho ốc M3×10 tự ren (ăn ren 5 mm)',
    'In úp mặt có nút xuống bàn, không cần support'] },
  { id: 'caps', name: 'Nắp nút ×12', layer: 'caps', color: 0xf08a24, facts: [
    'Ø10 mm; AIR, PREV, NEXT Ø12 mm; cao 3.5 mm',
    'Lỗ Ø3.6 sâu 2 mm ấn vào trục nút, thêm 1 giọt keo 502',
    'Nắp cách mặt hộp 1 mm, che kín lỗ hốc nút'] },
  { id: 'tact_switches', name: 'Nút nhấn 6×6×8 ×12', layer: 'top', color: 0x454a52, facts: [
    'Lắp TỪ MẶT TRÊN: ấn nút xuống hốc tới khi tì lên gờ đỡ',
    '4 chân xuyên qua lỗ gờ đỡ; hàn dây hoặc bẻ chân là nút không tuột ra được',
    'Nhấn nút chỉ ép nút xuống gờ đỡ, không cần keo',
    'Trục nhô 3 mm khỏi mặt hộp'] },
  { id: 'bottom_shell', name: 'Vỏ dưới', layer: 'bottom', shell: true, color: 0xc8ced6, facts: [
    'Rãnh chữ U ở hai đầu cho cổng USB-C, mở từ trên xuống',
    'Lỗ ngoài 13 × 7.5 mm cho đầu cáp lọt vào tận cổng',
    'Gờ đỡ, gờ dẫn hai bên và gờ chặn cho từng board',
    'Hốc công tắc gạt kín 4 phía, khe cần gạt ở cạnh trái',
    '4 lỗ ốc âm dưới đáy'] },
  { id: 'battery', name: 'Pin LiPo 1500 mAh', layer: 'parts', color: 0x9fb0c2, facts: [
    '51 × 34 × 8 mm, khoang 34.8 × 52.7 mm',
    'Gờ bốn phía giữ ngang; 2 cột trên nắp cách mặt pin 1.2 mm giữ không cho nhấc lên'] },
  { id: 'charger', name: 'Mạch sạc TP4056 Type-C', layer: 'parts', color: 0x2f6bd8, facts: [
    'Mạch 17.3 × 28 mm (cần đo lại), kê cao 1.5 mm',
    'Cổng USB-C thả vào rãnh ở đuôi, lưỡi khóa trên nắp ép xuống',
    'Gờ chặn phía sau đỡ lực khi cắm cáp, hở 0.3 mm'] },
  { id: 'esp32', name: 'ESP32-S3 SuperMini', layer: 'parts', color: 0x24282e, facts: [
    'Lắp ÚP: mặt có chân hướng lên, cổng USB-C nằm dưới mạch',
    'Đỡ đủ 4 cạnh: bệ dưới cổng USB, gờ ở đầu kia, 2 thanh đỡ bậc thang dọc 2 cạnh dài',
    'Các gờ chỉ chạm dải 0.6 mm sát mép mạch; linh kiện chúc xuống nằm lọt ở giữa',
    'Cổng USB-C lọt vào rãnh ở mũi, lưỡi khóa trên nắp ép xuống',
    'Mạch 18 × 22.6 mm (cần đo lại); mặt trên mạch cách nút bấm ~4 mm cho mối hàn và dây'] },
  { id: 'gyro', name: 'GY-521 (MPU6050)', layer: 'parts', color: 0x3d84c4, facts: [
    'Hốc 17 × 21.8 mm với 4 góc chặn hình chữ L, kê cao 1.5 mm',
    'Nằm phẳng, chip hướng lên, mũi tên X chỉ về đầu remote',
    'Cột trên nắp cách chip 0.4 mm giữ không cho nhấc lên'] },
  { id: 'power_switch', name: 'Công tắc gạt SS-12D01', layer: 'parts', color: 0x2b2f35, facts: [
    'Thả từ trên xuống: cần gạt vào khe, thân vào hốc',
    'Gờ chặn hai đầu và hai gờ phía sau (chừa chỗ 3 chân)',
    'Lưỡi khóa trên nắp đóng khe phía trên cần gạt'] },
];

// layers: pose per layer in mm (x = sideways, z = up; flip = turned over along the
// length). Layers left out stay in place.
// camera / target: world position (three.js is y-up; the front tip points to -z, so a
// camera at +z looks over the user's shoulder and the cap symbols read upright).
// "open" lays the lid upside down next to the base, as on the bench during assembly.
const LID_BESIDE_BASE = { x: 56, z: 21, flip: true };
export const MODES = {
  closed:   { layers: {}, section: false, camera: [-130, 170, 185], target: [0, 0, 0] },
  open:     { layers: { top: LID_BESIDE_BASE, caps: LID_BESIDE_BASE }, section: false,
              camera: [-75, 235, 200], target: [28, 0, 0] },
  exploded: { layers: { bottom: { z: -16 }, top: { z: 30 }, caps: { z: 50 } }, section: false,
              camera: [-180, 135, 150], target: [0, 0, 0] },
  section:  { layers: {}, section: true, camera: [285, 165, 140], target: [0, 0, 0] },
};
