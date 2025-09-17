# ESP32 Remote Access Control System

A secure Ethernet-based remote access control system with integrated web server and real-time monitoring capabilities. Features WebSocket communication, NTP synchronization, comprehensive logging, and mDNS discovery for easy network access.

## 🚀 Features

### Core Functionality
- **Ethernet Connectivity**: DHCP or static IP configuration with mDNS support
- **Web Server**: Built-in secure web interface with authentication
- **Real-time Updates**: WebSocket integration for live status and log updates
- **NTP Time Sync**: Accurate date/time synchronization with Argentina timezone
- **Log History**: Comprehensive action logging with timestamps (max 100 entries)
- **Relay Control**: Hardware relay operation with state monitoring
- **Session Management**: Secure token-based authentication system
- **Configuration Interface**: Web-based system configuration and user management

### Web Interface Features
- **Authentication**: Secure login with configurable user credentials
- **Admin Fallback**: Always-available admin access (admin/admin123)
- **Real-time Updates**: Live door status and log updates via WebSocket
- **Control Operations**: Web-based open/close/toggle relay operations
- **Log History**: View recent actions with date/time stamps
- **Network Configuration**: DHCP/static IP settings via web interface
- **User Management**: Change credentials through web interface
- **Responsive Design**: Mobile-friendly interface in Spanish

### Security Features
- **Session Tokens**: Secure authentication with auto-expiring sessions
- **Admin Recovery**: Never get locked out with admin credentials
- **Input Validation**: Secure form handling and validation
- **File Security**: Configuration stored securely in LittleFS

## 📁 Project Structure

```
├── data/
│   ├── config.json          # Main configuration file
│   ├── user_config.json     # User credentials (runtime)
│   ├── network_config.json  # Network settings (runtime)
│   └── logs.json           # Action log history (runtime)
├── include/                 # Header files
│   ├── board.h             # Hardware pin definitions
│   ├── ConfigLoader.h      # JSON configuration loader
│   ├── EthernetModule.h    # Ethernet functionality
│   ├── NetworkController.h # Network management
│   └── WebServerModule.h   # Web server and WebSocket
├── lib/                    # Custom libraries (empty)
├── src/                    # Source files
│   ├── main.cpp           # Main application entry point
│   ├── ConfigLoader.cpp   # Configuration implementation
│   ├── EthernetModule.cpp # Ethernet implementation
│   ├── NetworkController.cpp # Network controller
│   └── WebServerModule.cpp # Web server implementation
├── test/                   # Test files
├── platformio.ini         # PlatformIO configuration
├── .gitignore            # Git ignore rules
└── README.md             # This file
```

## 🔗 API Endpoints

### Authentication Endpoints
- `GET /` - Login page (redirects to /control if authenticated)
- `POST /login` - User authentication
- `GET /logout` - Session termination

### Control Endpoints
- `GET /control` - Main control interface (requires authentication)
- `POST /control` - Relay control operations (open/close/toggle)
- `GET /open` - Direct relay open (requires authentication)
- `GET /close` - Direct relay close (requires authentication)

### Configuration Endpoints
- `GET /config` - System configuration page (requires authentication)
- `POST /config` - Update system settings (network/user credentials)
- `GET /logs` - View complete log history (requires authentication)

### WebSocket Endpoint
- `WS /ws` - Real-time updates for status and logs

## 🔐 Authentication & Sessions

### Session Token System
- **Token Generation**: 32-character random hex string
- **Token Storage**: Server-side session management
- **Token Validation**: Required for all protected endpoints
- **Session Timeout**: Automatic cleanup on logout

### User Credentials
- **Configurable Users**: Change via web interface
- **Admin Fallback**: `admin` / `admin123` always available
- **Password Storage**: Secure file-based storage in LittleFS
- **Credential Validation**: Server-side authentication

### Security Features
- **Session Protection**: All sensitive operations require valid session
- **Input Validation**: Form data validation and sanitization
- **Admin Recovery**: Never get locked out of the system
- **Secure Storage**: Configuration files stored securely

## ⚙️ Configuration

### Main Configuration (data/config.json)
```json
{
  "ethernet": {
    "mac": "DE:AD:BE:EF:FE:ED",
    "hostname": "esp32-relay"
  }
}
```

### Runtime Configuration Files

#### User Credentials (data/user_config.json)
```json
{
  "username": "admin",
  "password": "admin123"
}
```
- **Auto-created** on first run
- **Modifiable** via web interface
- **Admin fallback** always available

#### Network Settings (data/network_config.json)
```json
{
  "dhcp": true,
  "ip": "192.168.1.100",
  "gateway": "192.168.1.1",
  "subnet": "255.255.255.0",
  "dns1": "8.8.8.8"
}
```
- **Auto-created** on first run
- **Modifiable** via web interface
- **Persists** across reboots

#### Log History (data/logs.json)
```json
[
  {"timestamp": "2025-01-17 14:30:15", "action": "ABRIR"},
  {"timestamp": "2025-01-17 14:31:20", "action": "CERRAR"}
]
```
- **Auto-generated** by system
- **Max 100 entries** (oldest removed automatically)
- **NTP synchronized** timestamps

## 🛠️ Setup Instructions

### 1. Clone Repository
```bash
git clone <repository-url>
cd esp32-access-control
```

### 2. Configure Hardware
Update `data/config.json` with your Ethernet MAC address:
```json
{
  "ethernet": {
    "mac": "YOUR:MAC:ADDRESS",
    "hostname": "esp32-relay"
  }
}
```

### 3. Build and Upload
```bash
# Build the project
pio run

# Upload to ESP32
pio run --target upload

# Upload filesystem (configuration)
pio run --target uploadfs
```

### 4. Access Web Interface
After successful upload, access the device via:
```
http://esp32-relay.local
```
**Or via IP address:**
```
http://169.254.x.x  (DHCP mode)
http://[configured-ip]  (Static IP mode)
```

### 5. Initial Login
- **Default credentials**: `admin` / `admin`
- **Admin fallback**: `admin` / `admin123` (always available)
- Configure your own credentials via the web interface

### 6. Network Configuration
- Access `/config` to set up network settings
- Choose DHCP or static IP configuration
- Settings persist across reboots

## 🔧 Development

### Adding New Features
1. Update configuration files with new options
2. Add corresponding getters in `ConfigLoader.h/.cpp`
3. Implement functionality in appropriate module
4. Test thoroughly and commit changes

### Code Style
- Use consistent naming conventions
- Add comments for complex logic
- Handle error conditions gracefully
- Follow ESP32 Arduino best practices

## 📊 Monitoring & Access

### Web Interface Access
- **mDNS**: `http://esp32-relay.local`
- **DHCP IP**: `http://169.254.x.x` (link-local)
- **Static IP**: `http://[configured-ip]`

### Web Interface Features
- **Real-time Status**: Live door status updates via WebSocket
- **Log History**: View recent actions with NTP timestamps
- **Network Config**: DHCP/static IP configuration
- **User Management**: Change credentials securely
- **Relay Control**: Web-based open/close/toggle operations

### Serial Output Monitoring
The device provides detailed serial output:
- ✅ Configuration loading status
- ✅ Network connection events
- ✅ Web server startup status
- ✅ NTP synchronization status
- ✅ Relay control operations
- ✅ WebSocket connection events
- ✅ Authentication attempts
- ✅ Log file operations

### Debug Information
- **Network Config**: DHCP/static IP settings loaded
- **User Config**: Credential loading status
- **Log System**: File load/save operations
- **WebSocket**: Connection/disconnection events
- **NTP**: Time synchronization status

## 🐛 Troubleshooting

### Common Issues

**Web Interface Not Accessible**
- ✅ Check Ethernet cable connection
- ✅ Verify mDNS: `http://esp32-relay.local`
- ✅ Check IP via serial output
- ✅ Try direct IP access if mDNS fails
- ✅ Ensure web server started successfully

**Direct Ethernet Connection Issues**
- ✅ Use crossover cable if connecting directly to PC
- ✅ Disable WiFi on laptop to avoid routing conflicts
- ✅ Check Windows Firewall settings
- ✅ Try different Ethernet ports

**NTP Time Sync Fails**
- ✅ Check Ethernet connectivity
- ✅ Verify NTP server accessibility (pool.ntp.org)
- ✅ Monitor serial output for NTP errors
- ✅ Check timezone configuration (ART3)

**WebSocket Updates Not Working**
- ✅ Check browser console for JavaScript errors
- ✅ Ensure WebSocket connection established
- ✅ Verify firewall allows WebSocket connections
- ✅ Try refreshing the page

**Configuration Not Saving**
- ✅ Check LittleFS space: `pio run --target uploadfs`
- ✅ Verify JSON syntax in configuration files
- ✅ Check serial output for save errors
- ✅ Ensure proper file permissions

**Static IP Not Working**
- ✅ Verify network_config.json was saved correctly
- ✅ Check IP address format and validity
- ✅ Ensure gateway/subnet are correct for network
- ✅ Reboot device after configuration changes

### Debug Commands
```bash
# Monitor serial output
pio device monitor

# Clean and rebuild
pio run --target clean
pio run

# Upload filesystem only
pio run --target uploadfs

# Check LittleFS usage
# (Monitor serial output after boot)
```

### Network Debugging
```bash
# Test mDNS resolution
ping esp32-relay.local

# Check Ethernet connection (Windows)
ipconfig /all | findstr Ethernet

# Check Ethernet connection (Linux/Mac)
ifconfig | grep eth
```

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly on ESP32 hardware
5. Submit a pull request

## 📄 License

This project is open source. Please check the license file for details.

## 📞 Support

For issues and questions:
- ✅ Check the troubleshooting section
- ✅ Review serial output for error messages
- ✅ Verify Ethernet connection and configuration
- ✅ Test mDNS resolution: `ping esp32-relay.local`
- ✅ Ensure configuration files are properly uploaded

### Quick Access Methods:
- **mDNS**: `http://esp32-relay.local`
- **DHCP**: `http://169.254.x.x` (direct connection)
- **Static**: `http://[configured-ip]` (network configured)

---

**Secure Remote Access Control System - Ready for Production! 🔐⚡**