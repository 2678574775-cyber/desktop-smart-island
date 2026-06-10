/*
 * gpio4_test.ino — 单独测试 GPIO4 引脚
 * 不碰 HC138，不初始化任何地址线
 * 上传后串口看 GPIO4 是否能正确读 HIGH
 *
 * 接线要求：暂时把 GPIO4 从电路中断开（或确保没有外部信号拉低它）
 * 如果无法断开，至少确保按键没有按住
 */

#define KEY_SENSE 3

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n==== GPIO4 独立测试 ====");
  Serial.println("测试1: INPUT_PULLUP，应该读 1 (HIGH)");
  Serial.println("如果读 0 → GPIO4 被外部拉低或有短路\n");

  pinMode(KEY_SENSE, INPUT_PULLUP);
  for (int i = 0; i < 10; i++) {
    int v = digitalRead(KEY_SENSE);
    Serial.printf("  digitalRead(GPIO4) = %d  (%s)\n", v, v ? "HIGH" : "LOW");
    delay(200);
  }

  Serial.println("\n测试2: 输出模式，驱动 HIGH/LOW，验证引脚本身没坏");
  pinMode(KEY_SENSE, OUTPUT);
  digitalWrite(KEY_SENSE, HIGH);
  Serial.printf("  驱动 HIGH，再设 INPUT_PULLUP 读: ");
  pinMode(KEY_SENSE, INPUT_PULLUP);
  delay(50);
  Serial.println(digitalRead(KEY_SENSE) ? "1 (OK)" : "0 (异常)");

  digitalWrite(KEY_SENSE, LOW); // 先输出 LOW
  pinMode(KEY_SENSE, OUTPUT);
  digitalWrite(KEY_SENSE, LOW);
  Serial.printf("  驱动 LOW，再设 INPUT_PULLUP 读: ");
  pinMode(KEY_SENSE, INPUT_PULLUP);
  delay(50);
  // 外部没有上拉的话应该读 0（因为引脚上有残余电荷或浮空，不一定准）
  // 这个测试主要是看能不能切换模式
  Serial.println(digitalRead(KEY_SENSE) ? "1" : "0");

  Serial.println("\n测试3: 用万用表量 GPIO4 电压");
  Serial.println("  INPUT_PULLUP 模式，不接任何东西 → 应该 3.3V");
  Serial.println("  如果 < 0.5V → 引脚短路到 GND 或外部被拉低\n");

  pinMode(KEY_SENSE, INPUT_PULLUP);
  Serial.println("现在进入循环读取，请用万用表量 GPIO4 电压...");
  Serial.println("按 Ctrl+C 停止上传新程序来保持此状态\n");
}

void loop() {
  static unsigned long last = 0;
  if (millis() - last > 500) {
    last = millis();
    Serial.printf("GPIO4 = %d  (时间=%lu s)\n", digitalRead(KEY_SENSE), millis()/1000);
  }
}
