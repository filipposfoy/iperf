import matplotlib.pyplot as plt
import numpy as np
import os

# Ensure directory exists
os.makedirs('network_plots', exist_ok=True)

# Data
timestamps = np.arange(0, 15)
goodput = [929.456, 929.935, 930.117, 930.204, 930.256, 
           930.286, 930.316, 930.333, 930.349, 930.361,
           930.371, 930.377, 930.379, 930.385, 930.393]
throughput = [967.578, 968.077, 968.266, 968.357, 968.411,
              968.443, 968.474, 968.491, 968.508, 968.520,
              968.531, 968.537, 968.539, 968.546, 968.553]
jitter = [16.134, 16.171, 16.189, 16.196, 16.198,
          16.201, 16.205, 16.209, 16.212, 16.214,
          16.214, 16.213, 16.210, 16.208, 16.207]

# Create figure with 3 subplots
plt.figure(figsize=(15, 12))

# 1. Combined Goodput/Throughput Plot
plt.subplot(3, 1, 1)
plt.plot(timestamps, goodput, label='Goodput (Mbps)', marker='o', color='blue')
plt.plot(timestamps, throughput, label='Throughput (Mbps)', marker='s', color='red')
plt.title('Network Performance Metrics Over Time')
plt.ylabel('Mbps')
plt.legend()
plt.grid(True)

# 2. Jitter Plot
plt.subplot(3, 1, 2)
plt.plot(timestamps, jitter, marker='D', color='green')
plt.ylabel('Jitter (μs)')
plt.ylim(16.12, 16.22)
plt.grid(True)

# 3. Efficiency Bar Chart
plt.subplot(3, 1, 3)
efficiency = [g/t*100 for g,t in zip(goodput, throughput)]
plt.bar(timestamps, efficiency, color='purple')
plt.axhline(y=np.mean(efficiency), color='black', linestyle='--', label=f'Avg: {np.mean(efficiency):.2f}%')
plt.ylabel('Efficiency (%)')
plt.xlabel('Time (seconds)')
plt.legend()
plt.grid(True)

plt.tight_layout()
plt.savefig('network_plots/combined_metrics.png')
plt.close()

# Individual Plots
# Goodput/Throughput only
plt.figure(figsize=(10, 5))
plt.plot(timestamps, goodput, label='Goodput (Mbps)', marker='o')
plt.plot(timestamps, throughput, label='Throughput (Mbps)', marker='s')
plt.xlabel('Time (seconds)')
plt.ylabel('Mbps')
plt.title('Throughput Comparison')
plt.legend()
plt.grid(True)
plt.savefig('network_plots/throughput_comparison.png')
plt.close()

# Jitter only
plt.figure(figsize=(10, 5))
plt.plot(timestamps, jitter, marker='D', color='green')
plt.xlabel('Time (seconds)')
plt.ylabel('Jitter (μs)')
plt.title('Packet Delay Variation')
plt.grid(True)
plt.ylim(16.12, 16.22)
plt.savefig('network_plots/jitter_analysis.png')
plt.close()

# Print metrics
print("=== Network Performance Metrics ===")
print(f"Average Goodput: {np.mean(goodput):.2f} Mbps")
print(f"Average Throughput: {np.mean(throughput):.2f} Mbps")
print(f"Average Efficiency: {np.mean(efficiency):.2f}%")
print(f"Average Overhead: {100-np.mean(efficiency):.2f}%")
print(f"Jitter Stability: {np.std(jitter):.4f} μs std.dev.")
print("\nPlots saved to /network_plots directory")