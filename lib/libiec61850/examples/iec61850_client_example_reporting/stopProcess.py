import os
import subprocess

# Range of service instances to delete
start = 1
end = 6

base_name = "report-client@{}.service"
unit_path_base = "/etc/systemd/system"

def delete_services():
    for i in range(start, end + 1):
        service = base_name.format(i)
        print(f"🛑 Stopping and disabling {service}...")

        subprocess.run(["systemctl", "stop", service], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        subprocess.run(["systemctl", "disable", service], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

        unit_file_path = os.path.join(unit_path_base, service)
        if os.path.exists(unit_file_path):
            os.remove(unit_file_path)
            print(f"🗑️ Deleted {unit_file_path}")
        else:
            print(f"⚠️ File not found: {unit_file_path}")

    print("🔄 Reloading systemd...")
    subprocess.run(["systemctl", "daemon-reload"])
    print("✅ Done!")

if __name__ == "__main__":
    if os.geteuid() != 0:
        print("⚠️ This script must be run as root.")
        exit(1)
    delete_services()
