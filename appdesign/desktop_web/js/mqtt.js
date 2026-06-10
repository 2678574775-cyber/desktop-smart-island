/* ==========================================================
   桌面智能灵动岛 — MQTT 客户端
   直连 Mosquitto，主题匹配 ESP32 固件的 desk/island/ 前缀
   用法：双击 index.html 用 file:// 打开或 Live Server
   ========================================================== */

const MqttClient = (function() {

  // --- 配置 ---
  var _host = 'localhost';
  var _port = 8083;
  var _client = null;
  var _connected = false;
  var _deviceOnline = false;

  // 主题（和 ESP32 固件保持一致）
  var TOPIC = {
    STATUS:   'desk/island/status',    // ESP32 → 网页：设备状态上报
    DISPLAY:  'desk/island/display',   // 网页 → ESP32：OLED 显示内容
    CMD:      'desk/island/cmd',       // 网页 → ESP32：控制指令
    TASK_ADD: 'desk/island/task/add',  // 网页 → ESP32：添加任务
    TASK_DONE:'desk/island/task/done', // 网页 → ESP32：任务完成
    POMODORO: 'desk/island/pomodoro',  // 网页 → ESP32：番茄钟控制
    CHAT:     'desk/island/chat',      // 网页 → ESP32：对话推送
  };

  // ==================== 内部 ====================

  function _notify(state) {
    window.dispatchEvent(new CustomEvent('mqtt-state', {
      detail: { state: state, connected: _connected, deviceOnline: _deviceOnline },
    }));
  }

  function _doConnect() {
    if (typeof mqtt === 'undefined') {
      console.error('[MQTT] mqtt.js 未加载，2s 后重试');
      setTimeout(_doConnect, 2000);
      return;
    }

    if (_client) { try { _client.end(true); } catch (_) {} }

    var url = 'ws://' + _host + ':' + _port;
    console.log('[MQTT] 连接', url);

    _client = mqtt.connect(url, {
      clientId: 'web-' + Math.random().toString(16).slice(2, 10),
      clean: true,
      connectTimeout: 6000,
      reconnectPeriod: 0,
    });

    _client.on('connect', function() {
      _connected = true;
      console.log('[MQTT] ✅ 已连接');
      // 订阅 ESP32 上报
      _client.subscribe('desk/island/status', { qos: 1 });
      _client.subscribe('desk/island/#', { qos: 1 });
      _notify('connected');
    });

    _client.on('message', function(topic, message) {
      var payload;
      try { payload = JSON.parse(message.toString()); } catch (_) { return; }

      console.log('[MQTT] 📥', topic, payload);

      // ESP32 上报状态 → 标记设备在线
      if (topic === TOPIC.STATUS) {
        if (payload.online !== undefined) _deviceOnline = payload.online;
        else _deviceOnline = true; // 发了消息就说明在线
        _notify('device_online');
      }

      window.dispatchEvent(new CustomEvent('mqtt-message', {
        detail: { topic: topic, payload: payload },
      }));
    });

    _client.on('error', function(err) {
      _connected = false;
      console.warn('[MQTT] 错误:', err.message);
      _notify('error');
    });

    _client.on('close', function() {
      _connected = false;
      _notify('disconnected');
      // 5 秒后重连
      setTimeout(_doConnect, 5000);
    });
  }

  // ==================== 公开 API ====================

  return {

    get connected() { return _connected; },
    get deviceOnline() { return _deviceOnline; },

    /** 修改 Broker 地址并重连 */
    setBroker: function(host, port) {
      _host = host;
      _port = port;
      localStorage.setItem('mqtt_host', host);
      localStorage.setItem('mqtt_port', String(port));
      if (_client) { try { _client.end(true); } catch (_) {} _client = null; }
      _connected = false;
      _doConnect();
    },

    /** 启动 */
    autoConnect: function() {
      var h = localStorage.getItem('mqtt_host');
      var p = localStorage.getItem('mqtt_port');
      if (h) _host = h;
      if (p) _port = parseInt(p) || 8083;
      _doConnect();
    },

    /** 发送到 ESP32 显示屏 */
    display: function(json) {
      if (!_connected) return false;
      _client.publish(TOPIC.DISPLAY, JSON.stringify(json), { qos: 1 });
      console.log('[MQTT] 📤', TOPIC.DISPLAY, json);
      return true;
    },

    /** 发送控制指令 */
    cmd: function(json) {
      if (!_connected) return false;
      _client.publish(TOPIC.CMD, JSON.stringify(json), { qos: 1 });
      console.log('[MQTT] 📤', TOPIC.CMD, json);
      return true;
    },

    /** 发布任务 */
    taskAdd: function(task) {
      if (!_connected) return false;
      _client.publish(TOPIC.TASK_ADD, JSON.stringify(task), { qos: 1 });
      return true;
    },

    taskDone: function(taskId, done) {
      if (!_connected) return false;
      _client.publish(TOPIC.TASK_DONE, JSON.stringify({ task_id: taskId, done: done }), { qos: 1 });
      return true;
    },

    /** 发布番茄钟 */
    pomodoro: function(action) {
      if (!_connected) return false;
      _client.publish(TOPIC.POMODORO, JSON.stringify({ action: action }), { qos: 1 });
      return true;
    },

    /** 推送对话 */
    chat: function(content) {
      if (!_connected) return false;
      _client.publish(TOPIC.CHAT, JSON.stringify({ content: content }), { qos: 1 });
      return true;
    },
  };

})();

console.log('🐕 MQTT 就绪  Mosquitto ws://localhost:8083  desk/island/#');
