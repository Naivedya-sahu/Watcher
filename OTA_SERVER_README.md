# Watcher OTA Server Setup

## Quick Start (3 steps)

### Step 1: Build firmware
```bash
cd d:\Development\Personal\Watcher\v7.0(DEPRECATED)\v7.1
idf.py build
# Creates: build/watcher.bin
```

### Step 2: Start OTA server
Choose one method:

**Option A: Batch file (Windows cmd)**
```cmd
cd build
start_ota_server.bat
```

**Option B: PowerShell**
```powershell
cd build
.\start_ota_server.ps1
```

**Option C: Direct Python**
```bash
cd build
python start_ota_server.py
```

### Step 3: Update device
1. Open: `http://watcher.local/` (or device IP)
2. In Settings → OTA URL, enter your server IP:
   ```
   https://192.168.1.50/watcher.bin
   ```
   (Replace 192.168.1.50 with your computer's IP)
3. Click **[Save]**
4. Click **[OTA Update]**
5. Device reboots with new firmware ✓

---

## Hostname Setup (Optional)

To use `watcher.ota` instead of IP address:

### Windows: Add to hosts file
1. Open: `C:\Windows\System32\drivers\etc\hosts` (as Administrator)
2. Add line at end:
   ```
   127.0.0.1    watcher.ota
   ```
3. Save file

Then use in OTA URL:
```
https://watcher.ota/watcher.bin
```

### From Device: Add to DNS config
Device can resolve `watcher.ota` if your router supports it or mDNS responder is running.

---

## Server Details

### What the scripts do:
1. ✓ Checks for `watcher.bin` in current directory
2. ✓ Generates self-signed SSL certificate (first run only)
3. ✓ Starts HTTPS server on port 443
4. ✓ Shows URLs and waits for requests
5. ✓ Logs each download with timestamp

### Certificate files (auto-generated):
- `cert.pem` - Self-signed certificate
- `key.pem` - Private key
- Safe to delete and regenerate anytime

### Port: 443 (HTTPS)
- Requires **Administrator/sudo** on Windows
- If port 443 unavailable, modify script to use 8443 and update device OTA URL

---

## Troubleshooting

### ❌ "Permission denied on port 443"
**Solution:** Run as Administrator
- Batch: Right-click `start_ota_server.bat` → Run as administrator
- PowerShell: Run PowerShell as administrator first, then `.\start_ota_server.ps1`

### ❌ "Python not found"
**Solution:** Use ESP-IDF's built-in Python
```bash
# ESP-IDF Python location:
C:\Espressif\tools\python\v6.0\venv\Scripts\python.exe start_ota_server.py
```

### ❌ Device can't reach server
**Troubleshooting:**
1. Verify IP on server:
   ```bash
   ipconfig  # Find your IP (e.g., 192.168.1.50)
   ```
2. Test from another device:
   ```bash
   curl -k https://192.168.1.50/watcher.bin --output test.bin
   ```
3. Check firewall allows port 443
4. Device and server on same WiFi network

### ❌ "watcher.bin not found"
**Solution:** Make sure you're in the `build/` directory
```bash
cd d:\Development\Personal\Watcher\v7.0(DEPRECATED)\v7.1\build
dir watcher.bin  # Should show file
```

---

## Advanced: Custom Port

To use port 8443 instead of 443 (no admin required):

Edit `start_ota_server.py`, change:
```python
PORT = 443
```
to:
```python
PORT = 8443
```

Then use in device OTA URL:
```
https://192.168.1.50:8443/watcher.bin
```

---

## Testing Without Device

Manually download firmware from server:
```bash
# From another terminal/computer:
curl -k https://192.168.1.50/watcher.bin -o test.bin
ls test.bin  # Should show downloaded file
```

---

## One-shot OTA trigger (API)

Once server is running, trigger OTA from device directly:

```bash
curl -X POST http://watcher.local/api/cmd \
  -H "Content-Type: application/json" \
  -d '{"cmd":"ota","url":"https://192.168.1.50/watcher.bin"}'
```

Response: `{"ok":true}` → Device starts downloading and flashing

---

## Production Deployment

For real deployments:
1. Use GitHub Releases to host `.bin` files
2. Use real SSL certificate (not self-signed)
3. Update device to verify certificate: remove `skip_cert_common_name_check=true` from ota_task.cpp

Current development setup: ✓ Self-signed OK
