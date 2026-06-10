/*
 * hc138_diag.ino — 74HC138 + 按键 裸机诊断
 * 不初始化 WiFi，不初始化 OLED，只做按键扫描
 * 上传后打开串口监视器 115200bps
 *
 * 接线检查：
 *   GPIO0 -> HC138 A0
 *   GPIO1 -> HC138 A1
 *   GPIO2 -> HC138 A2
 *   GPIO3 -> 按键公共端（通过 R6 10k）
 *   HC138 G1 -> 3V3
 *   HC138 G2A -> GND
 *   HC138 G2B -> GND
 */

#define KEY_A0   0
#define KEY_A1   1
#define KEY_A2   2
#define KEY_SENSE 3

// 扫描顺序：[A,B,C,D,E,F] = [Y5,Y0,Y3,Y4,Y2,Y1]
const uint8_t scanAddr[6] = { 0b101, 0b000, 0b011, 0b100, 0b010, 0b001 };
const char* btnName[6] = { "A/SW6", "B/SW1", "C/SW4", "D/SW5", "E/SW3", "F/SW2" };

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n==== HC138 按键诊断 ====");

  // 只初始化按键相关引脚
  pinMode(KEY_A0, OUTPUT);
  pinMode(KEY_A1, OUTPUT);
  pinMode(KEY_A2, OUTPUT);
  digitalWrite(KEY_A0, HIGH);
  digitalWrite(KEY_A1, HIGH);
  digitalWrite(KEY_A2, HIGH);

  pinMode(KEY_SENSE, INPUT_PULLUP);

  Serial.println("Step 1: 空闲扫描（不按任何键）");
  Serial.println("格式: addr(Yx) sense → 应该全部是 1");
  for (int i = 0; i < 6; i++) {
    uint8_t addr = scanAddr[i];
    digitalWrite(KEY_A0, addr & 0x01);
    digitalWrite(KEY_A1, (addr >> 1) & 0x01);
    digitalWrite(KEY_A2, (addr >> 2) & 0x01);
    delayMicroseconds(10);
    uint8_t val = digitalRead(KEY_SENSE);
    Serial.printf("  [%d] addr=%d(Y%s) KEY_SENSE=%d\n", i, addr,
      i == 0 ? "5" : i == 1 ? "0" : i == 2 ? "3" : i == 3 ? "4" : i == 4 ? "2" : "1",
      val);
  }

  Serial.println("\nStep 2: 逐键测试");
  Serial.println("请按顺序按每个键，观察哪一行变成 KEY_SENSE=0");
  Serial.println("格式: 按B键 → 第[1]行应该变成 0\n");

  // 打印表头
  Serial.print("         ");
  for (int i = 0; i < 6; i++) {
    Serial.print(btnName[i][0]); Serial.print("    ");
  }
  Serial.println();
}

void loop() {
  Serial.print("t=");
  Serial.print(millis() / 1000);
  Serial.print("s  ");

  for (int i = 0; i < 6; i++) {
    uint8_t addr = scanAddr[i];
    digitalWrite(KEY_A0, addr & 0x01);
    digitalWrite(KEY_A1, (addr >> 1) & 0x01);
    digitalWrite(KEY_A2, (addr >> 2) & 0x01);
    delayMicroseconds(10);
    uint8_t val = digitalRead(KEY_SENSE);
    Serial.print(val == LOW ? "1   " : "0   ");
  }
  Serial.println();
  delay(200);
}
