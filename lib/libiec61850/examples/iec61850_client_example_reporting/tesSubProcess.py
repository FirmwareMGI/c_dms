import os
import subprocess
import time

# Configuration
base_service_name = "report-client"
binary_path = "/home/pi/c_dms/lib/libiec61850/examples/iec61850_client_example_reporting/client_example_reporting"
config_dir = "/home/pi/c_dms/lib/libiec61850/examples/iec61850_client_example_reporting/"
systemd_dir = "/etc/systemd/system"
user = "pi"

# Instances to create and monitor
clients = {
    "1": "config1.json",
    "2": "config2.json",
    "3": "config3.json",
    "4": "config4.json",
    "5": "config5.json",
}

# Template for systemd unit
unit_template = """[Unit]
Description=Report Client Instance {instance}
After=network.target

[Service]
Type=simple
ExecStart={binary} {config_dir}/{config_file} {instance}
WorkingDirectory={config_dir}
Restart=always
RestartSec=3
User={user}
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target
"""

def write_unit_file(instance, config_file):
    unit_name = f"{base_service_name}@{instance}.service"
    unit_path = os.path.join(systemd_dir, unit_name)
    content = unit_template.format(
        instance=instance,
        binary=binary_path,
        config_dir=config_dir,
        config_file=config_file,
        user=user
    )
    with open(unit_path, "w") as f:
        f.write(content)
    print(f"✅ Created {unit_path}")

def reload_and_enable_start(instance):
    service = f"{base_service_name}@{instance}"
    subprocess.run(["systemctl", "daemon-reload"], check=True)
    subprocess.run(["systemctl", "enable", service], check=True)
    subprocess.run(["systemctl", "start", service], check=True)
    print(f"🚀 Enabled and started {service}")

def check_service_status(instance):
    service = f"{base_service_name}@{instance}"
    result = subprocess.run(
        ["systemctl", "is-active", service],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )
    return result.stdout.strip()

def main():
    # Must be root
    if os.geteuid() != 0:
        print("⚠️ This script must be run as root.")
        exit(1)

    # Create and start services
    for instance, config_file in clients.items():
        write_unit_file(instance, config_file)
        reload_and_enable_start(instance)

    print("🔁 Monitoring service statuses. Press Ctrl+C to stop.")
    try:
        while True:
            for instance in clients.keys():
                status = check_service_status(instance)
                print(f"🔎 {base_service_name}@{instance} → {status}")
            time.sleep(5)  # Delay between checks
    except KeyboardInterrupt:
        print("\n🛑 Monitoring stopped by user.")

if __name__ == "__main__":
    main()
