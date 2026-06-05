/* ==========================================================
   桌面智能灵动岛 — MQTT 客户端（WebSocket）
   使用 mqtt.js 库，通过 EMQX WebSocket 端口连接
   加载方式：index.html 中通过 CDN 引入 mqtt.js
   ========================================================== */

const MqttClient = (() => {
  // --- 默认配置 ---
  const DEFAULT_BROKER = 'broker.emqx.io';
  const DEFAULT_WS_PORT = 8083;       // EMQX WebSocket 端口
  const DEFAULT_TCP_PORT = 1883;      // TCP 端口（仅用于显示）
  const DISCOVER_TOPIC = 'desktoppet/discover';

  // --- 内部状态 ---
  let _client = null;                 // mqtt.js client 实例
  let _deviceId = '';
  let _connected = false;
  let _brokerUrl = '';
  let _handlers = [];                 // [{ topic, fn }]
  let _reconnectTimer = null;
  let _reconnectAttempts = 0;

  // --- 公开 API ---
  return {
    /** 是否已连接 */
    get connected() { return _connected; },

    /** 当前设备 ID */
    get deviceId() { return _deviceId; },

    /** Broker WebSocket URL */
    get brokerUrl() { return _brokerUrl; },

    /**
     * 连接到 MQTT Broker
     * @param {Object} opts - { host, port, deviceId, username, password }
     *                         port 默认 8083 (WebSocket)
     * @returns {Promise<boolean>}
     */
    async connect(opts = {}) {
      const host = opts.host || DEFAULT_BROKER;
      const port = opts.port || DEFAULT_WS_PORT;
      const deviceId = opts.deviceId || '';

      // 如果已连接，先断开
      if (_client) {
        await this.disconnect();
      }

      _deviceId = deviceId;
      _brokerUrl = `ws://${host}:${port}/mqtt`;
      // 注意：mqtt.js 要求 ws:// 或 wss:// 协议
      const wsUrl = port === 8084 || port === 8884
        ? `wss://${host}:${port}/mqtt`
        : (port === 8083 ? `ws://${host}:${port}/mqtt` : `ws://${host}:${port}/mqtt`);

      console.log('[MQTT] 正在连接', wsUrl);

      return new Promise((resolve) => {
        try {
          // mqtt.js 全局函数 mqtt.connect(url, options)
          _client = mqtt.connect(wsUrl, {
            clientId: 'web_' + Date.now().toString(36),
            keepalive: 30,
            clean: true,
            username: opts.username || undefined,
            password: opts.password || undefined,
            will: deviceId ? {
              topic: `device/${deviceId}/status`,
              payload: JSON.stringify({ online: false }),
              qos: 1,
              retain: false,
            } : undefined,
            connectTimeout: 5000,
            reconnectPeriod: 0,  // 我们自己管理重连
          });

          _client.on('connect', () => {
            _connected = true;
            _reconnectAttempts = 0;
            console.log('[MQTT] ✅ 已连接');
            _subscribeDeviceTopics();
            // 通知所有监听者
            _notifyState('connected');
            resolve(true);
          });

          _client.on('message', (topic, message) => {
            try {
              const payload = JSON.parse(message.toString());
              console.log('[MQTT] 📥', topic, payload);

              // 分发给匹配的处理器
              for (const h of _handlers) {
                if (h.fn && matchTopic(h.topic, topic)) {
                  h.fn(topic, payload);
                }
              }

              // 全局消息事件
              window.dispatchEvent(new CustomEvent('mqtt-message', {
                detail: { topic, payload },
              }));
            } catch (e) {
              console.warn('[MQTT] 消息解析失败:', topic, e);
            }
          });

          _client.on('error', (err) => {
            console.error('[MQTT] ❌ 错误:', err.message || err);
            _connected = false;
            _notifyState('error');
          });

          _client.on('close', () => {
            _connected = false;
            console.log('[MQTT] 连接关闭');
            _notifyState('disconnected');
            _scheduleReconnect(host, port, deviceId, opts);
          });

          _client.on('offline', () => {
            _connected = false;
            console.log('[MQTT] 离线');
            _notifyState('disconnected');
          });

          // 连接超时
          setTimeout(() => {
            if (!_connected) {
              console.warn('[MQTT] 连接超时');
              resolve(false);
            }
          }, 6000);

        } catch (e) {
          console.error('[MQTT] 连接异常:', e);
          _connected = false;
          resolve(false);
        }
      });
    },

    /** 断开连接 */
    async disconnect() {
      _reconnectTimer && clearTimeout(_reconnectTimer);
      _reconnectTimer = null;
      if (_client) {
        try {
          _client.end(true);
        } catch (_) {}
        _client = null;
      }
      _connected = false;
      _notifyState('disconnected');
      console.log('[MQTT] 已断开');
    },

    /**
     * 订阅主题
     * @param {string} topic - 支持通配符 + (单层) # (多层)
     * @param {Function} fn - 回调 (topic, payload) => {}
     */
    subscribe(topic, fn) {
      if (_client && _connected) {
        _client.subscribe(topic, { qos: 1 }, (err) => {
          if (err) {
            console.warn('[MQTT] 订阅失败:', topic, err);
          } else {
            console.log('[MQTT] 已订阅:', topic);
          }
        });
      }
      // 即使当前未连接也记录处理器，重连后重新订阅
      _handlers.push({ topic, fn });
    },

    /**
     * 取消订阅
     * @param {string} topic
     * @param {Function} [fn] - 如果提供，只移除该回调
     */
    unsubscribe(topic, fn) {
      if (fn) {
        _handlers = _handlers.filter(h => !(h.topic === topic && h.fn === fn));
      } else {
        _handlers = _handlers.filter(h => h.topic !== topic);
      }
      if (_client && _connected) {
        _client.unsubscribe(topic);
      }
    },

    /**
     * 发布消息
     * @param {string} topic
     * @param {Object} payload
     * @param {Object} [opts] - { qos, retain }
     */
    publish(topic, payload, opts = {}) {
      if (!_client || !_connected) {
        console.warn('[MQTT] ⚠️ 未连接，无法发布到', topic);
        return false;
      }
      const msg = JSON.stringify(payload);
      _client.publish(topic, msg, {
        qos: opts.qos || 1,
        retain: opts.retain || false,
      });
      console.log('[MQTT] 📤', topic, payload);
      return true;
    },

    // --- 业务快捷方法 ---

    /** 发布设备发现广播 */
    publishDiscover() {
      return this.publish(DISCOVER_TOPIC, {
        from: 'web_client',
        timestamp: new Date().toISOString(),
      });
    },

    /** 发布任务添加 */
    publishTaskAdd(task) {
      return this.publish(`device/${_deviceId}/task/add`, task);
    },

    /** 发布任务完成状态 */
    publishTaskDone(taskId, done) {
      return this.publish(`device/${_deviceId}/task/done`, {
        task_id: taskId,
        done: done,
        timestamp: new Date().toISOString(),
      });
    },

    /** 推送消息到设备 */
    publishChatMessage(content) {
      return this.publish(`device/${_deviceId}/message/chat`, {
        content: content,
        timestamp: new Date().toISOString(),
      });
    },

    /** 发布番茄钟控制 */
    publishPomodoro(action) {
      return this.publish(`device/${_deviceId}/pomodoro/start`, {
        action: action,
        timestamp: new Date().toISOString(),
      });
    },

    /** 更新设备 ID */
    async setDeviceId(newId) {
      if (newId === _deviceId) return;
      // 取消旧订阅
      if (_deviceId && _client && _connected) {
        _client.unsubscribe(`device/${_deviceId}/#`);
      }
      _deviceId = newId;
      // 重新订阅
      await _subscribeDeviceTopics();
    },

    /** 获取发现的所有设备 */
    getDiscoveredDevices() {
      return _discoveredDevices;
    },

    /** 开始设备发现扫描 */
    startDiscovery(onDeviceFound) {
      _discoveredDevices = [];
      _discoveryCallback = onDeviceFound;

      // 订阅所有设备的 discover/ack
      this.subscribe('device/+/discover/ack', (topic, payload) => {
        const device = {
          id: payload.id || '',
          name: payload.name || '未知设备',
          ip: payload.ip || '',
          battery: payload.battery || 0,
          online: payload.online || false,
          state: payload.state || 'idle',
          petMood: payload.pet_mood || 'happy',
          version: payload.version || '',
        };

        if (!device.id) return;

        // 去重
        const existingIdx = _discoveredDevices.findIndex(d => d.id === device.id);
        if (existingIdx >= 0) {
          _discoveredDevices[existingIdx] = device;
        } else {
          _discoveredDevices.push(device);
        }

        // 按电量排序
        _discoveredDevices.sort((a, b) => b.battery - a.battery);

        if (_discoveryCallback) {
          _discoveryCallback(_discoveredDevices);
        }
      });

      // 发送广播
      this.publishDiscover();

      // 5 秒后自动停止
      _discoveryTimer && clearTimeout(_discoveryTimer);
      _discoveryTimer = setTimeout(() => {
        this.stopDiscovery();
      }, 5000);

      return true;
    },

    /** 停止设备发现 */
    stopDiscovery() {
      _discoveryTimer && clearTimeout(_discoveryTimer);
      this.unsubscribe('device/+/discover/ack');
      console.log('[MQTT] 设备发现结束，共发现', _discoveredDevices.length, '台');
    },

    /** 重置 */
    reset() {
      _handlers = [];
      _discoveredDevices = [];
      _deviceId = '';
      this.disconnect();
    },
  };

  // ========== 内部函数 ==========

  let _discoveredDevices = [];
  let _discoveryCallback = null;
  let _discoveryTimer = null;

  /** 订阅当前设备的所有主题 */
  async function _subscribeDeviceTopics() {
    if (!_client || !_connected || !_deviceId) return;

    const topics = [
      `device/${_deviceId}/status`,
      `device/${_deviceId}/task/list`,
      `device/${_deviceId}/task/done`,
      `device/${_deviceId}/pomodoro/status`,
      `device/${_deviceId}/chat/history`,
      `device/${_deviceId}/discover/ack`,
    ];

    for (const topic of topics) {
      _client.subscribe(topic, { qos: 1 }, (err) => {
        if (err) console.warn('[MQTT] 订阅失败:', topic, err);
        else console.log('[MQTT] 已订阅:', topic);
      });
    }

    // 也订阅广播发现应答
    _client.subscribe('device/+/discover/ack', { qos: 1 }, (err) => {
      if (!err) console.log('[MQTT] 已订阅: 设备发现应答');
    });
  }

  /** 简单主题匹配（支持 + 单层通配，# 多层通配） */
  function matchTopic(pattern, topic) {
    const regexStr = pattern
      .replace(/[.+^${}()|[\]\\]/g, '\\$&')  // 转义特殊字符
      .replace(/\\\+/g, '[^/]+')              // + → 单层
      .replace(/#/g, '.*');                   // # → 多层
    return new RegExp('^' + regexStr + '$').test(topic);
  }

  /** 通知状态变化 */
  function _notifyState(state) {
    window.dispatchEvent(new CustomEvent('mqtt-state', {
      detail: { state, deviceId: _deviceId },
    }));
  }

  /** 自动重连 */
  function _scheduleReconnect(host, port, deviceId, opts) {
    if (_reconnectTimer) return;
    const delay = Math.min(
      (_reconnectAttempts < 5 ? Math.pow(2, _reconnectAttempts) : 30) * 1000,
      30000
    );
    _reconnectAttempts++;
    console.log(`[MQTT] 🔄 ${delay / 1000}s 后重连 (第 ${_reconnectAttempts} 次)`);

    _reconnectTimer = setTimeout(async () => {
      _reconnectTimer = null;
      await MqttClient.connect({ host, port, deviceId, ...opts });
    }, delay);
  }
})();

console.log('🐕 MQTT 客户端已就绪 (WebSocket)');
