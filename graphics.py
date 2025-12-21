import matplotlib.pyplot as plt

# ========================
# Dados experimentais
# ========================

# 1. FER
fer = [1, 10, 25, 50]        # %
eff_fer = [22.9, 11.5, 8.3, 6.5]

# 2. Propagation Time
tprop = [10, 25, 50]          # ms
eff_tprop = [0.230, 0.228, 0.224]

# 3. Baudrate
baud = [4800, 9600, 19200,38400]    # bps
eff_baud = [46.5, 23.2, 11.6, 5.8]

# 4. Frame Size
frame = [500, 2000, 3000, 4000]     # bits
eff_frame = [12.6, 31.3, 26.3, 27.1]

# ========================
# Gráfico combinado (2x2)
# ========================

fig, axs = plt.subplots(2, 2, figsize=(10, 8))

# FER
axs[0, 0].plot(fer, eff_fer, marker='o', linewidth=2)
axs[0, 0].set_title("Efficiency vs Frame Error Rate")
axs[0, 0].set_xlabel("FER (%)")
axs[0, 0].set_ylabel("Efficiency (S)")
axs[0, 0].grid(True, linestyle='--', alpha=0.6)

# Propagation Time
axs[0, 1].plot(tprop, eff_tprop, marker='o', linewidth=2)
axs[0, 1].set_title("Efficiency vs Propagation Time")
axs[0, 1].set_xlabel("Tprop (ms)")
axs[0, 1].set_ylabel("Efficiency (S)")
axs[0, 1].grid(True, linestyle='--', alpha=0.6)

# Baudrate
axs[1, 0].plot(baud, eff_baud, marker='o', linewidth=2)
axs[1, 0].set_title("Efficiency vs Baudrate")
axs[1, 0].set_xlabel("Baudrate (bps)")
axs[1, 0].set_ylabel("Efficiency (S)")
axs[1, 0].grid(True, linestyle='--', alpha=0.6)

# Frame Size
axs[1, 1].plot(frame, eff_frame, marker='o', linewidth=2)
axs[1, 1].set_title("Efficiency vs Frame Size")
axs[1, 1].set_xlabel("Frame Size (bits)")
axs[1, 1].set_ylabel("Efficiency (S)")
axs[1, 1].grid(True, linestyle='--', alpha=0.6)

# Layout compacto
plt.tight_layout(rect=[0, 0, 1, 0.97])
plt.savefig("efficiency_all_params.png", dpi=300)
plt.show()
