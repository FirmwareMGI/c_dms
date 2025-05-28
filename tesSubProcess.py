import subprocess
import time
import signal

# Dictionary to track running processes
processes = {}

# List of argument sets
arg_sets = {
    "proc1": ["my_program", "1"],
    "proc2": ["my_program", "2"],
    "proc3": ["my_program", "3"]
}

# Path to the compiled C program
program_path = './my_program'

# Start all processes
def start_all():
    for name, args in arg_sets.items():
        if name in processes and processes[name].poll() is None:
            print(f"{name} is already running.")
        else:
            proc = subprocess.Popen([program_path] + args)
            processes[name] = proc
            print(f"Started {name} with PID {proc.pid}")

# Stop a specific process
def stop_process(name):
    proc = processes.get(name)
    print(f"Stopping {name}...")
    if proc and proc.poll() is None:
        proc.terminate()  # or proc.kill()
        print(f"Stopped {name}")
    else:
        print(f"{name} is not running.")

# Restart a specific process
def restart_process(name):
    stop_process(name)
    time.sleep(0.5)  # Give it time to shut down
    args = arg_sets[name]
    proc = subprocess.Popen([program_path] + args)
    processes[name] = proc
    print(f"Restarted {name} with PID {proc.pid}")

# Example usage:
start_all()
time.sleep(5)  # Let them run a bit

stop_process("proc2")
stop_process("proc1")
stop_process("proc3")

time.sleep(2)

restart_process("proc2")
