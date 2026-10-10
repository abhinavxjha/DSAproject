# ⚡ Smart Energy Consumption Tracker

> **Desktop C Application using GTK+ 3.0 GUI and Academic Data Structures**  
> A practical energy auditing and electricity bill estimation desktop tool developed from scratch in C.

---

## 📋 Table of Contents
1. [Project Overview](#-project-overview)
2. [Key Features](#-key-features)
3. [Academic DSA Foundation](#-academic-dsa-foundation)
4. [Mathematical & Energy Calculation Model](#-mathematical--energy-calculation-model)
5. [Project File Structure](#-project-file-structure)
6. [How to Run the Application](#-how-to-run-the-application)
   - [Method 1: One-Click Launch (Easiest - No Terminal)](#method-1-one-click-launch-easiest--no-terminal)
   - [Method 2: Windows PowerShell](#method-2-windows-powershell)
   - [Method 3: Windows Command Prompt (CMD)](#method-3-windows-command-prompt-cmd)
   - [Method 4: MSYS2 MinGW64 Shell & Makefile](#method-4-msys2-mingw64-shell--makefile)
7. [How to Compile / Rebuild](#-how-to-compile--rebuild)
8. [Benchmark Test Case (Hostel Room Example)](#-benchmark-test-case-hostel-room-example)
9. [Project Limitations & PBL Context](#-project-limitations--pbl-context)

---

## 🌟 Project Overview

**Smart Energy Consumption Tracker** is a native desktop application written in **C** using the **GTK+ 3.0** GUI toolkit. It allows hostel students, homeowners, and small office administrators to:
* Track electrical appliances in their rooms/offices.
* Estimate **daily** and **monthly energy consumption (kWh / units)**.
* Estimate the **monthly electricity bill (in ₹)** based on customizable per-unit electricity tariffs.
* Identify the **highest energy-consuming appliance** and visualize its share of total consumption.
* Compare appliance loads using an in-place **Merge Sort** algorithm.
* Save and reload appliance configurations to a local plain text file (`data.txt`).

---

## 🚀 Key Features

* **GUI Desktop Interface:** Clean GTK+ 3 interface with custom CSS styling, card containers, responsive tree view, and modal dialogs.
* **Appliance Inventory (CRUD):** Add, view, edit, and delete appliances in real time.
* **Dynamic Settings:** Instantly adjust electricity rate (default ₹8.00/unit) and calculation period (default 30 days) to recalculate all rows and summaries.
* **DSA Search:** Locate any appliance quickly by numeric ID or name (case-insensitive substring match).
* **DSA Sorting:** Sort appliances descending by energy consumption using an in-place Recursive Merge Sort.
* **Dynamic Energy Summary:** Real-time metrics for total units, daily kWh, monthly kWh, estimated bill, and top consumer percentage.
* **Explicit Persistence:** Appliances only save to `data.txt` when the user explicitly clicks **Save (data.txt)**.
* **Robust Input Validation:** Rejects empty names, negative/zero wattage, zero quantity, and daily hours outside 0.1–24.0.

---

## 🧠 Academic DSA Foundation

This project naturally applies core Data Structures and Algorithms concepts:

| Concept | Implementation in Project | Complexity |
| :--- | :--- | :--- |
| **Singly Linked List** | Appliances stored as dynamically allocated nodes (`Appliance*`) linked via pointers | Space: $O(N)$ |
| **Dynamic Memory Allocation** | Heap allocation and reclamation with `malloc()`, `free()`, and `free_appliance_list()` | Zero memory leaks |
| **Manual Merge Sort** | Fast & slow pointer bisection (Tortoise & Hare) and recursive in-place merging | Time: $O(N \log N)$<br>Aux Space: $O(1)$ |
| **Linear Search** | Traverses linked list matching node `id == query_id` or case-insensitive string match | Time: $O(N)$ |
| **Linear Scan** | Single-pass accumulator to compute total metrics and identify highest consumer | Time: $O(N)$ |
| **File Handling** | Formatted token parsing (`fopen`, `fgets`, `strtok`, `fprintf`, `fclose`) with `data.txt` | $O(N)$ records |

---

## 📐 Mathematical & Energy Calculation Model

1. **Power in Kilowatts:**
   $$\text{Power (kW)} = \frac{\text{Power (Watts)}}{1000}$$

2. **Daily Energy Consumption:**
   $$\text{Daily Energy (kWh)} = \text{Power (kW)} \times \text{Quantity} \times \text{Hours Per Day}$$

3. **Monthly Energy Consumption:**
   $$\text{Monthly Energy (kWh)} = \text{Daily Energy (kWh)} \times \text{Billing Days}$$

4. **Estimated Electricity Bill:**
   $$\text{Estimated Bill (₹)} = \text{Monthly Energy (kWh)} \times \text{Rate per kWh}$$

5. **Top Consumer Percentage Share:**
   $$\text{Share (\%)} = \left(\frac{\text{Top Appliance Daily kWh}}{\text{Total Daily kWh}}\right) \times 100$$

*(Note: $1\text{ kWh} = 1\text{ electricity unit}$. Default: ₹8.00/unit, 30 billing days).*

---

## 📁 Project File Structure

```
SmartEnergyTracker/
│
├── main.c        # GTK+ 3 GUI frontend: window, layouts, table, and user callbacks
├── energy.c      # Pure C backend: linked list operations, Merge Sort, calculations, file I/O
├── energy.h      # Header file: structs, constants, and function declarations
├── data.txt      # Plain text persistence file for settings and appliances
├── Makefile      # Build script for GCC and GTK3 compilation
├── run.bat       # 1-click Windows launcher (sets runtime DLL path automatically)
└── README.md     # Project documentation and run guide
```

---

## 💻 How to Run the Application

The compiled executable `SmartEnergyTracker.exe` requires the GTK3 runtime DLLs (`C:\msys64\mingw64\bin`). You can run it using any of the following methods:

### Method 1: One-Click Launch (Easiest — No Terminal)
1. Open Windows File Explorer and navigate to `e:\DSAproject2`.
2. **Double-click `run.bat`**.
3. The desktop GUI application will open immediately!

---

### Method 2: Windows PowerShell
Open Windows PowerShell in the project directory and run:
```powershell
$env:Path = "C:\msys64\mingw64\bin;" + $env:Path
.\SmartEnergyTracker.exe
```

---

### Method 3: Windows Command Prompt (CMD)
Open Command Prompt in `e:\DSAproject2` and run:
```cmd
set PATH=C:\msys64\mingw64\bin;%PATH%
SmartEnergyTracker.exe
```

---

### Method 4: MSYS2 MinGW64 Shell & Makefile
Open the MSYS2 MinGW 64-bit shell (`C:\msys64\msys2_shell.cmd -mingw64`) and run:
```bash
cd /e/DSAproject2
make run
```

---

## 🔨 How to Compile / Rebuild

If you modify [`main.c`](file:///e:/DSAproject2/main.c), [`energy.c`](file:///e:/DSAproject2/energy.c), or [`energy.h`](file:///e:/DSAproject2/energy.h), recompile using either:

### Via Makefile (Recommended):
In MSYS2 MinGW64 terminal:
```bash
cd /e/DSAproject2
make clean
make
```

### Via Direct GCC Command:
```bash
gcc -Wall -Wextra -std=c99 -I. -O2 $(pkg-config --cflags gtk+-3.0) main.c energy.c -o SmartEnergyTracker.exe $(pkg-config --libs gtk+-3.0) -lm
```

---

## 🧪 Benchmark Test Case (Hostel Room Example)

To test the accuracy of the application, enter the following sample hostel room appliances:

| Appliance Name | Quantity | Power (W) | Usage (Hours/Day) | Daily Energy (kWh) | Monthly Energy (30 days) | Est. Cost (@ ₹8/unit) |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: |
| **Ceiling Fan** | 2 | 75 W | 10.0 hrs | 1.500 kWh | 45.00 kWh | ₹360.00 |
| **LED Bulb** | 3 | 9 W | 6.0 hrs | 0.162 kWh | 4.86 kWh | ₹38.88 |
| **Air Conditioner** | 1 | 1500 W | 6.0 hrs | 9.000 kWh | 270.00 kWh | ₹2,160.00 |
| **Laptop Charger** | 1 | 65 W | 4.0 hrs | 0.260 kWh | 7.80 kWh | ₹62.40 |
| **Mobile Charger** | 2 | 10 W | 1.0 hr | 0.020 kWh | 0.60 kWh | ₹4.80 |

### Expected Summary Results:
* **Total Registered:** 5 appliances (9 units)
* **Total Daily Energy:** `10.942 kWh / day`
* **Total Monthly Energy:** `328.26 kWh / month`
* **Estimated Monthly Bill:** `₹ 2,626.08`
* **Highest Consumer:** `Air Conditioner (9.000 kWh/day - 82.3% of total bill)`

---

## ⚠️ Project Limitations & PBL Context

* **Estimation Nature:** This application calculates energy based on rated appliance wattage and average user-specified hours. Real-world consumption differs because compressors (ACs, refrigerators) cycle on and off based on thermostats.
* **Billing Tiers:** Real utility bills often include progressive slab tariffs, fixed meter charges, and fuel surcharges.
* **PBL Educational Goal:** The project fulfills Problem-Based Learning (PBL) by creating an intuitive tool that builds consumer awareness regarding phantom loads and high-consumption appliances.
