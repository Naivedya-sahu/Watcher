#!/usr/bin/env python3
"""
Simple HTTPS server for Watcher OTA firmware updates.
Serves watcher.bin from current directory over HTTPS.
Certificate: self-signed (OK for local development)
"""

import os
import ssl
import sys
import socket
import shutil
import functools
import subprocess
import http.server
from datetime import datetime, timedelta, timezone
from pathlib import Path

class QuietHTTPRequestHandler(http.server.SimpleHTTPRequestHandler):
    def log_message(self, format, *args):
        """Log HTTP requests with timestamps"""
        print(f"[{self.log_date_time_string()}] {format % args}")

def get_local_ips():
    """Get all local IP addresses"""
    ips = []
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(("8.8.8.8", 80))
        local_ip = s.getsockname()[0]
        s.close()
        ips.append(local_ip)
    except:
        pass
    ips.append("127.0.0.1")
    return ips

def create_self_signed_cert():
    """Create self-signed SSL certificate if it doesn't exist"""
    cert_file = "cert.pem"
    key_file = "key.pem"
    
    if os.path.exists(cert_file) and os.path.exists(key_file):
        print(f"✓ Using existing certificate: {cert_file}")
        return cert_file, key_file
    
    print("Generating self-signed certificate...")

    # Preferred path: use OpenSSL if available.
    openssl = shutil.which("openssl")
    if openssl:
        try:
            result = subprocess.run(
                [
                    openssl,
                    "req",
                    "-x509",
                    "-newkey",
                    "rsa:2048",
                    "-keyout",
                    key_file,
                    "-out",
                    cert_file,
                    "-days",
                    "365",
                    "-nodes",
                    "-subj",
                    "/CN=watcher.ota",
                ],
                check=False,
                capture_output=True,
                text=True,
            )
            if result.returncode == 0 and os.path.exists(cert_file) and os.path.exists(key_file):
                print(f"✓ Certificate created with OpenSSL: {cert_file}, {key_file}")
                return cert_file, key_file
            print("⚠ OpenSSL certificate generation failed; trying Python fallback")
        except Exception:
            print("⚠ OpenSSL certificate generation failed; trying Python fallback")

    # Fallback: generate cert via Python cryptography (works on Windows without OpenSSL).
    try:
        from cryptography import x509
        from cryptography.hazmat.primitives import hashes, serialization
        from cryptography.hazmat.primitives.asymmetric import rsa
        from cryptography.x509.oid import NameOID

        key = rsa.generate_private_key(public_exponent=65537, key_size=2048)
        subject = issuer = x509.Name(
            [
                x509.NameAttribute(NameOID.COUNTRY_NAME, "IN"),
                x509.NameAttribute(NameOID.ORGANIZATION_NAME, "Watcher Dev"),
                x509.NameAttribute(NameOID.COMMON_NAME, "watcher.ota"),
            ]
        )
        now = datetime.now(timezone.utc)
        cert = (
            x509.CertificateBuilder()
            .subject_name(subject)
            .issuer_name(issuer)
            .public_key(key.public_key())
            .serial_number(x509.random_serial_number())
            .not_valid_before(now - timedelta(minutes=1))
            .not_valid_after(now + timedelta(days=365))
            .add_extension(
                x509.SubjectAlternativeName([
                    x509.DNSName("watcher.ota"),
                    x509.DNSName("localhost"),
                    x509.IPAddress(__import__("ipaddress").ip_address("127.0.0.1")),
                ]),
                critical=False,
            )
            .sign(key, hashes.SHA256())
        )

        with open(key_file, "wb") as fk:
            fk.write(
                key.private_bytes(
                    encoding=serialization.Encoding.PEM,
                    format=serialization.PrivateFormat.TraditionalOpenSSL,
                    encryption_algorithm=serialization.NoEncryption(),
                )
            )

        with open(cert_file, "wb") as fc:
            fc.write(cert.public_bytes(serialization.Encoding.PEM))

        print(f"✓ Certificate created with Python cryptography: {cert_file}, {key_file}")
        return cert_file, key_file
    except ImportError:
        print("⚠ Python package 'cryptography' is not installed.")
        print("  Install with: python -m pip install cryptography")
    except Exception as e:
        print(f"⚠ Python certificate fallback failed: {e}")
    
    print("⚠ Could not create certificate automatically")
    print("  Option 1: install OpenSSL and rerun")
    print("  Option 2: python -m pip install cryptography and rerun")
    print("  Option 3: manually create cert.pem and key.pem in project root")
    return None, None

def main():
    script_dir = Path(__file__).resolve().parent
    build_dir = script_dir / "build"
    firmware_path = build_dir / "watcher.bin"
    os.chdir(script_dir)
    
    # Check for watcher.bin under build/
    if not firmware_path.exists():
        print("❌ ERROR: watcher.bin not found at build/watcher.bin")
        print(f"   Expected path: {firmware_path}")
        print("   Run: idf.py build")
        sys.exit(1)
    
    print("=" * 70)
    print("WATCHER OTA HTTPS SERVER")
    print("=" * 70)
    print(f"📁 Script directory: {script_dir}")
    print(f"📁 Serving directory: {build_dir}")
    print(f"📦 Firmware file: watcher.bin ({firmware_path.stat().st_size:,} bytes)")
    print()
    
    # Create certificate
    cert_file, key_file = create_self_signed_cert()
    if not cert_file or not key_file:
        print("❌ Certificate files not found and could not be created")
        sys.exit(1)
    
    # Setup server
    PORT = 443
    try:
        handler = functools.partial(QuietHTTPRequestHandler, directory=str(build_dir))
        server = http.server.HTTPServer(("0.0.0.0", PORT), handler)
    except PermissionError:
        print(f"❌ Permission denied on port {PORT} (requires admin)")
        print("   Try: python start_ota_server.py (as Administrator)")
        sys.exit(1)
    
    # Setup SSL
    context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    context.load_cert_chain(cert_file, key_file)
    server.socket = context.wrap_socket(server.socket, server_side=True)
    
    # Show access info
    local_ips = get_local_ips()
    print("🌐 Server running on:")
    for ip in local_ips:
        print(f"   https://{ip}/watcher.bin")
    print()
    print("🔒 Certificate: self-signed (warnings normal)")
    print("📱 Preferred OTA URL: https://watcher.ota/watcher.bin")
    print()
    print("Press Ctrl+C to stop server...")
    print("=" * 70)
    print()
    
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\n" + "=" * 70)
        print("✓ Server stopped")
        print("=" * 70)

if __name__ == "__main__":
    main()
