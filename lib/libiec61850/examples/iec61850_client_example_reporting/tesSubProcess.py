# import subprocess
# import time
# import signal
# import sys

# # Dictionary to track running processes
# processes = {}

# # List of argument sets
# arg_sets = {
#     "reportClient1": ["config1.json", "1"],
#     "reportClient2": ["config2.json", "2"],
#     "reportClient3": ["config3.json", "3"]
# }

# # Path to the compiled C program
# program_path = './client_example_reporting'

# # Start all processes
# def start_all():
#     for name, args in arg_sets.items():
#         if name in processes and processes[name].poll() is None:
#             print(f"{name} is already running.")
#         else:
#             proc = subprocess.Popen([program_path] + args)
#             processes[name] = proc
#             print(f"Started {name} with PID {proc.pid}")

# # Stop a specific process
# def stop_process(name):
#     proc = processes.get(name)
#     print(f"Stopping {name}...")
#     if proc and proc.poll() is None:
#         proc.terminate()  # You can also use proc.kill() if needed
#         proc.wait()
#         print(f"Stopped {name}")
#     else:
#         print(f"{name} is not running.")

# # Stop all running processes
# def stop_all():
#     print("Stopping all running processes...")
#     for name in list(processes.keys()):
#         stop_process(name)
#     print("All processes stopped.")

# # Restart a specific process
# def restart_process(name):
#     stop_process(name)
#     time.sleep(0.5)  # Give it time to shut down
#     args = arg_sets[name]
#     proc = subprocess.Popen([program_path] + args)
#     processes[name] = proc
#     print(f"Restarted {name} with PID {proc.pid}")

# # === MAIN EXECUTION ===
# if __name__ == "__main__":
#     while True:
#         try:
#             start_all()
#             print("🟢 All processes started. Press Ctrl+C to stop.")
#             time.sleep(100)  # Let them run
#         except KeyboardInterrupt:
#             print("\n🔴 KeyboardInterrupt received. Cleaning up...")
#         finally:
#             stop_all()
#             print("✅ Program exited cleanly.")


import os
import subprocess

# Configuration
base_service_name = "report-client"
binary_path = "/home/pi/c_dms/lib/libiec61850/examples/iec61850_client_example_reporting/client_example_reporting"
config_dir = "/home/pi/c_dms/lib/libiec61850/examples/iec61850_client_example_reporting/"
systemd_dir = "/etc/systemd/system"
user = "pi"  # <-- Change this to your system user

# Instances to create
clients = {
    "1": "config1.json"
}

# Template content for the systemd unit
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

def main():
    for instance, config_file in clients.items():
        write_unit_file(instance, config_file)
        reload_and_enable_start(instance)

if __name__ == "__main__":
    if os.geteuid() != 0:
        print("⚠️ This script must be run as root.")
        exit(1)
    main()
