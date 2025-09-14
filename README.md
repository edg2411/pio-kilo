# ESP32 IoT Project with MQTT and WiFi Support

A comprehensive ESP32 IoT project featuring JSON-based configuration, MQTT communication, WiFi static IP support, and robust network management.

## 🚀 Features

### Core Functionality
- **JSON Configuration**: All settings loaded from `data/config.json`
- **MQTT Communication**: Secure MQTT with configurable topics and automatic reconnection
- **WiFi Management**: DHCP or static IP configuration with failover support
- **Network Resilience**: Automatic reconnection with rate limiting
- **Command Handling**: Bidirectional MQTT communication with command callbacks

### Communication Features
- **Heartbeat**: Periodic status updates (30s intervals)
- **Sensor Data**: Simulated sensor readings (10s intervals)
- **Status Updates**: System status reports (60s intervals)
- **Command Reception**: Remote command handling with callbacks

### Network Architecture
- **Priority-based Connection**: WiFi → Ethernet → LTE
- **Graceful Hardware Handling**: Works with missing Ethernet/LTE hardware
- **Static IP Support**: Configurable static IP for WiFi
- **SSL/TLS Security**: Certificate-based MQTT authentication

## 📁 Project Structure

```
├── data/
│   ├── config.json          # Main configuration file
│   └── config.json.example  # Configuration template
├── include/                 # Header files
│   ├── ConfigLoader.h      # JSON configuration loader
│   ├── MQTTModule.h        # MQTT communication module
│   ├── NetworkController.h # Network management
│   ├── WiFiModule.h        # WiFi functionality
│   └── board.h             # Hardware pin definitions
├── lib/                    # Custom libraries (empty)
├── src/                    # Source files
│   ├── main.cpp           # Main application
│   ├── ConfigLoader.cpp   # Configuration implementation
│   ├── MQTTModule.cpp     # MQTT implementation
│   ├── NetworkController.cpp # Network controller
│   ├── WiFiModule.cpp     # WiFi implementation
│   └── *.cpp              # Other module implementations
├── test/                   # Test files
├── platformio.ini         # PlatformIO configuration
├── .gitignore            # Git ignore rules
└── README.md             # This file
```

## ⚙️ Configuration

### WiFi Configuration
```json
"wifi": {
  "ssid": "YourWiFiSSID",
  "password": "YourWiFiPassword",
  "staticIP": {
    "enabled": true,
    "ip": "192.168.1.150",
    "gateway": "192.168.1.1",
    "subnet": "255.255.255.0",
    "dns1": "8.8.8.8",
    "dns2": "8.8.4.4"
  }
}
```

### MQTT Configuration
```json
"mqtt": {
  "broker": "your-mqtt-broker.com",
  "port": 8883,
  "clientId": "esp32-client",
  "username": "your-username",
  "password": "your-password",
  "topics": {
    "status": "home/status",
    "command": "home/command",
    "sensor": "home/sensor",
    "heartbeat": "home/heartbeat"
  }
}
```

## 🛠️ Setup Instructions

### 1. Clone and Configure
```bash
git clone <repository-url>
cd esp32-iot-project
```

### 2. Configure WiFi and MQTT
```bash
# Copy example configuration
cp data/config.json.example data/config.json

# Edit configuration with your settings
nano data/config.json
```

### 3. Upload Configuration
```bash
# Upload filesystem data to ESP32
pio run --target uploadfs
```

### 4. Build and Upload
```bash
# Build the project
pio run

# Upload to ESP32
pio run --target upload
```

## 📡 MQTT Topics

### Publishing Topics
- `home/heartbeat`: Device heartbeat with uptime and status
- `home/sensor`: Sensor data (temperature, humidity, timestamp)
- `home/status`: System status updates

### Subscribing Topics
- `home/command`: Remote commands (JSON format)

### Example Command
```json
{
  "message": "hello",
  "action": "led_control",
  "value": true
}
```

## 🔧 Development

### Adding New Features
1. Update `config.json` with new configuration options
2. Add corresponding getters in `ConfigLoader.h/.cpp`
3. Implement functionality in appropriate module
4. Test and commit changes

### Code Style
- Use consistent naming conventions
- Add comments for complex logic
- Handle error conditions gracefully
- Follow ESP32 Arduino best practices

## 📊 Monitoring

### Serial Output
The device provides detailed serial output:
- Configuration loading status
- Network connection events
- MQTT connection/disconnection
- Message publishing confirmations
- Command reception confirmations

### Debug Levels
- `✅`: Success operations
- `❌`: Error conditions
- `ℹ️`: Informational messages

## 🐛 Troubleshooting

### Common Issues

**MQTT Connection Fails**
- Check broker credentials in `config.json`
- Verify SSL certificates are uploaded
- Ensure network connectivity

**WiFi Connection Issues**
- Verify SSID and password
- Check static IP configuration
- Monitor serial output for connection attempts

**Configuration Loading Fails**
- Ensure `data/config.json` is properly formatted
- Check LittleFS upload: `pio run --target uploadfs`
- Verify JSON syntax

### Debug Commands
```bash
# Monitor serial output
pio device monitor

# Clean and rebuild
pio run --target clean
pio run
```

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## 📄 License

This project is open source. Please check the license file for details.

## 📞 Support

For issues and questions:
- Check the troubleshooting section
- Review serial output for error messages
- Ensure all configuration files are properly uploaded

---

**Happy IoT Building! 🚀**