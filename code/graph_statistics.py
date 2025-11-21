import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import numpy as np

# Style

sns.set_style("whitegrid")
# Giữ kích thước Figure lớn
plt.rcParams['figure.figsize'] = (20, 16) 
# Giảm font size chung một chút cho khoảng cách
plt.rcParams['font.size'] = 9 

# Load CSV (Giả định 'csma_metrics.csv' tồn tại)
# df = pd.read_csv('csma_metrics.csv') 

# === Dữ liệu mẫu (Thay thế cho việc đọc file CSV nếu bạn không cung cấp file) ===
np.random.seed(42)
time_points = np.linspace(0, 10, 50)
df = pd.DataFrame({
    'Time': time_points,
    'TCP_Throughput_Mbps': 50 / (1 + time_points) + np.random.rand(50) * 0.5,
    'UDP_Throughput_Mbps': 5 + np.random.rand(50) * 0.5,
    'TCP_PDR_%': 99.8 + np.random.rand(50) * 0.1,
    'UDP_PDR_%': 100 - (1 + time_points) * 0.5 + np.random.rand(50) * 0.5,
    'TCP_Packets_Sent': 2500 * time_points + np.random.rand(50) * 50,
    'UDP_Packets_Sent': 5000 * time_points + np.random.rand(50) * 50,
    'TCP_Packets_Received': 2500 * time_points * (0.999), 
    'UDP_Packets_Received': 5000 * time_points * (1 - 0.005*time_points), 
    'TCP_Bytes_Received': 2500 * time_points * 1500 * (0.999),
    'UDP_Bytes_Received': 5000 * time_points * 1500 * (1 - 0.005*time_points),
})
# === Kết thúc Dữ liệu mẫu ===

# Clip PDR TCP để hiển thị hợp lý (nếu >100%)
df['TCP_PDR_%'] = np.clip(df['TCP_PDR_%'], 0, 100)

# Packet Loss không âm
df['TCP_Packets_Lost'] = np.maximum(df['TCP_Packets_Sent'] - df['TCP_Packets_Received'], 0)
df['UDP_Packets_Lost'] = np.maximum(df['UDP_Packets_Sent'] - df['UDP_Packets_Received'], 0)

# Figure với subplots
fig, axes = plt.subplots(3, 3, figsize=(20, 16)) 
axes = axes.flatten()

# 1️⃣ Throughput
axes[0].plot(df['Time'], df['TCP_Throughput_Mbps'], 'b-', label='TCP')
axes[0].plot(df['Time'], df['UDP_Throughput_Mbps'], 'r-', label='UDP')
axes[0].set_title('Throughput Comparison', fontsize=12) # Giảm font title
axes[0].set_xlabel('Time (s)'); axes[0].set_ylabel('Mbps')
axes[0].legend(); axes[0].grid(True, alpha=0.3)

# 2️⃣ PDR
axes[1].plot(df['Time'], df['TCP_PDR_%'], 'b-', label='TCP')
axes[1].plot(df['Time'], df['UDP_PDR_%'], 'r-', label='UDP')
axes[1].set_title('Packet Delivery Ratio', fontsize=12) # Giảm font title
axes[1].set_xlabel('Time (s)'); axes[1].set_ylabel('PDR (%)')
axes[1].legend(); axes[1].grid(True, alpha=0.3)
axes[1].set_ylim([0, 105])

# 3️⃣ Cumulative Packets Received
axes[2].plot(df['Time'], df['TCP_Packets_Received'], 'b-', label='TCP')
axes[2].plot(df['Time'], df['UDP_Packets_Received'], 'r-', label='UDP')
axes[2].set_title('Cumulative Packets Received', fontsize=12) # Giảm font title
axes[2].set_xlabel('Time (s)'); axes[2].set_ylabel('Packets')
axes[2].legend(); axes[2].grid(True, alpha=0.3)

# 4️⃣ Cumulative Bytes Received
axes[3].plot(df['Time'], df['TCP_Bytes_Received']/1024, 'b-', label='TCP')
axes[3].plot(df['Time'], df['UDP_Bytes_Received']/1024, 'r-', label='UDP')
axes[3].set_title('Cumulative Data Received', fontsize=12) # Giảm font title
axes[3].set_xlabel('Time (s)'); axes[3].set_ylabel('KB')
axes[3].legend(); axes[3].grid(True, alpha=0.3)

# 5️⃣ Packet Loss
axes[4].plot(df['Time'], df['TCP_Packets_Lost'], 'b-', label='TCP')
axes[4].plot(df['Time'], df['UDP_Packets_Lost'], 'r-', label='UDP')
axes[4].set_title('Packet Loss Over Time', fontsize=12) # Giảm font title
axes[4].set_xlabel('Time (s)'); axes[4].set_ylabel('Packets Lost')
axes[4].legend(); axes[4].grid(True, alpha=0.3)

# 6️⃣ Packets Sent
axes[5].plot(df['Time'], df['TCP_Packets_Sent'], 'b-', label='TCP')
axes[5].plot(df['Time'], df['UDP_Packets_Sent'], 'r-', label='UDP')
axes[5].set_title('Cumulative Packets Sent', fontsize=12) # Giảm font title
axes[5].set_xlabel('Time (s)'); axes[5].set_ylabel('Packets Sent')
axes[5].legend(); axes[5].grid(True, alpha=0.3)

# 7️⃣ Bar Chart - Final Metrics
metrics = ['Throughput (Mbps)', 'PDR (%)', 'Packets Received (x10k)'] 
tcp_vals = [df['TCP_Throughput_Mbps'].iloc[-1], df['TCP_PDR_%'].iloc[-1], df['TCP_Packets_Received'].iloc[-1]/10000]
udp_vals = [df['UDP_Throughput_Mbps'].iloc[-1], df['UDP_PDR_%'].iloc[-1], df['UDP_Packets_Received'].iloc[-1]/10000]
x = np.arange(len(metrics)); width = 0.35
axes[6].bar(x - width/2, tcp_vals, width, label='TCP', color='steelblue')
axes[6].bar(x + width/2, udp_vals, width, label='UDP', color='coral')
axes[6].set_title('Final Metrics Comparison', fontsize=12); axes[6].set_xticks(x); 
axes[6].set_xticklabels(metrics, rotation=0, fontsize=8) # Giảm font xticklabels
axes[6].legend(); axes[6].grid(True, alpha=0.3, axis='y')

# 8️⃣ Protocol Efficiency
tcp_eff = (df['TCP_Packets_Received']/df['TCP_Packets_Sent']*100).fillna(0)
udp_eff = (df['UDP_Packets_Received']/df['UDP_Packets_Sent']*100).fillna(0)
axes[7].fill_between(df['Time'], tcp_eff, alpha=0.3, color='blue', label='TCP')
axes[7].fill_between(df['Time'], udp_eff, alpha=0.3, color='red', label='UDP')
axes[7].plot(df['Time'], tcp_eff, 'b-', linewidth=2)
axes[7].plot(df['Time'], udp_eff, 'r-', linewidth=2)
axes[7].set_title('Protocol Efficiency Over Time', fontsize=12) # Giảm font title
axes[7].set_xlabel('Time (s)'); axes[7].set_ylabel('Efficiency (%)')
axes[7].legend(); axes[7].grid(True, alpha=0.3); axes[7].set_ylim([0,105])

# 9️⃣ Statistics Table
axes[8].axis('off')
stats_data = [
['Metric','TCP','UDP'],
['Avg Throughput (Mbps)', f"{df['TCP_Throughput_Mbps'].mean():.2f}", f"{df['UDP_Throughput_Mbps'].mean():.2f}"],
['Final PDR (%)', f"{df['TCP_PDR_%'].iloc[-1]:.2f}", f"{df['UDP_PDR_%'].iloc[-1]:.2f}"],
['Total Packets Sent', f"{int(df['TCP_Packets_Sent'].iloc[-1]):,}", f"{int(df['UDP_Packets_Sent'].iloc[-1]):,}"],
['Total Packets Received', f"{int(df['TCP_Packets_Received'].iloc[-1]):,}", f"{int(df['UDP_Packets_Received'].iloc[-1]):,}"],
['Total Data (KB)', f"{df['TCP_Bytes_Received'].iloc[-1]/1024:.2f}", f"{df['UDP_Bytes_Received'].iloc[-1]/1024:.2f}"]
]
table = axes[8].table(cellText=stats_data, cellLoc='center', loc='center', colWidths=[0.35,0.3,0.3])
table.auto_set_font_size(False); table.set_fontsize(9); table.scale(1,2)
for i in range(3): table[(0,i)].set_facecolor('#4472C4'); table[(0,i)].set_text_props(weight='bold', color='white')

# Overall title
fig.suptitle('CSMA Ethernet Performance: TCP vs UDP Analysis', fontsize=16, fontweight='bold', y=1.02) 

# ĐIỀU CHỈNH CHÍNH: Sử dụng subplots_adjust để tăng khoảng cách giữa các biểu đồ con
# wspace: Khoảng cách ngang (width space)
# hspace: Khoảng cách dọc (height space)
plt.subplots_adjust(hspace=0.35, wspace=0.3) 

plt.savefig('csma_performance_analysis_adjusted.png', dpi=300, bbox_inches='tight')
plt.show()
print("✓ Chart saved: csma_performance_analysis_adjusted.png")
