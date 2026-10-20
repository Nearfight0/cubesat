import cv2
import tkinter as tk
from tkinter import ttk, messagebox
from PIL import Image, ImageTk
import time
import os
import random


class F16CameraStation:
    def __init__(self, root):
        self.root = root
        self.root.title("F-16 MFD // CUBESAT GROUND CONTROL STATION")
        self.root.geometry("1150x720")

        # F-16 MFD Dark Palette
        self.COLOR_BG = "#060a08"         # Mat gece/kokpit siyahı
        self.COLOR_PANEL = "#0d1410"      # Panel çerçeveleri
        self.COLOR_HUD_GREEN = "#00ff66"  # F-16 Phosphor Green
        self.COLOR_DIM_GREEN = "#005522"  # Pasif/Sönük yeşil
        self.COLOR_AMBER = "#ffb000"      # İkaz sarısı
        self.COLOR_ALERT = "#ff3333"      # Tehlike kırmızısı
        self.COLOR_TEXT_DIM = "#4a6b53"   # İkincil metinler

        self.root.configure(bg=self.COLOR_BG)
        self.root.resizable(True, True)

        self.cap = None
        self.running = False
        self.recording = False
        self.video_writer = None
        self.current_frame = None
        self.fps_counter = 0
        self.fps_time = time.time()
        self.fps_display = 0

        # Logo nesneleri (Garbage Collector silmesin diye tutuluyor)
        self.logo_images = {}

        self._build_ui()
        self._load_logos()
        self._scan_cameras()

    def _build_ui(self):
        # ── TOP STATUS / HUD HEADER ──────────────────────────────────────────
        top = tk.Frame(self.root, bg=self.COLOR_PANEL, bd=1, relief=tk.SOLID, pady=6)
        top.pack(fill=tk.X, padx=8, pady=(8, 4))

        title_lbl = tk.Label(
            top, text="[ MFD-1: CUBESAT LIVE PAYLOAD FEED ]",
            bg=self.COLOR_PANEL, fg=self.COLOR_HUD_GREEN,
            font=("Consolas", 12, "bold")
        )
        title_lbl.pack(side=tk.LEFT, padx=12)

        self.rec_indicator = tk.Label(
            top, text="",
            bg=self.COLOR_PANEL, fg=self.COLOR_ALERT,
            font=("Consolas", 11, "bold")
        )
        self.rec_indicator.pack(side=tk.LEFT, padx=10)

        self.status_sys = tk.Label(
            top, text="SYS: STANDBY",
            bg=self.COLOR_PANEL, fg=self.COLOR_AMBER,
            font=("Consolas", 10, "bold")
        )
        self.status_sys.pack(side=tk.RIGHT, padx=12)

        self.fps_label = tk.Label(
            top, text="FPS: 00",
            bg=self.COLOR_PANEL, fg=self.COLOR_HUD_GREEN,
            font=("Consolas", 10, "bold")
        )
        self.fps_label.pack(side=tk.RIGHT, padx=12)

        # ── MFD BEZEL / CONTROLS ─────────────────────────────────────────────
        ctrl = tk.Frame(self.root, bg=self.COLOR_BG, pady=4)
        ctrl.pack(fill=tk.X, padx=8)

        tk.Label(
            ctrl, text="DEV_SEL>", bg=self.COLOR_BG, fg=self.COLOR_HUD_GREEN,
            font=("Consolas", 10, "bold")
        ).pack(side=tk.LEFT, padx=(4, 4))

        self.camera_var = tk.StringVar()

        style = ttk.Style()
        style.theme_use("clam")
        style.configure("TCombobox",
                        fieldbackground=self.COLOR_PANEL,
                        background=self.COLOR_DIM_GREEN,
                        foreground=self.COLOR_HUD_GREEN,
                        darkcolor=self.COLOR_BG,
                        lightcolor=self.COLOR_HUD_GREEN)

        self.camera_combo = ttk.Combobox(
            ctrl, textvariable=self.camera_var, width=24, state="readonly"
        )
        self.camera_combo.pack(side=tk.LEFT, padx=(0, 10))

        def create_osb_button(parent, text, command, fg_color=self.COLOR_HUD_GREEN):
            return tk.Button(
                parent, text=text, command=command,
                bg=self.COLOR_PANEL, fg=fg_color,
                activebackground=self.COLOR_DIM_GREEN, activeforeground=fg_color,
                font=("Consolas", 9, "bold"), bd=1, relief=tk.RAISED,
                padx=8, pady=2, cursor="hand2"
            )

        self.btn_start = create_osb_button(ctrl, "[ OSB 01: PWR ON ]", self.start_camera)
        self.btn_start.pack(side=tk.LEFT, padx=3)

        self.btn_stop = create_osb_button(ctrl, "[ OSB 02: PWR OFF ]", self.stop_camera, self.COLOR_ALERT)
        self.btn_stop.config(state=tk.DISABLED)
        self.btn_stop.pack(side=tk.LEFT, padx=3)

        self.btn_rec = create_osb_button(ctrl, "[ OSB 03: REC / SNAP ]", self.toggle_record_and_snap, self.COLOR_AMBER)
        self.btn_rec.config(state=tk.DISABLED)
        self.btn_rec.pack(side=tk.LEFT, padx=3)

        self.btn_scan = create_osb_button(ctrl, "[ RE-SCAN ]", self._scan_cameras)
        self.btn_scan.pack(side=tk.LEFT, padx=3)

        # ── MAIN DISPLAY CONTAINER (LOGOS + CANVAS) ──────────────────────────
        main_container = tk.Frame(self.root, bg=self.COLOR_BG)
        main_container.pack(expand=True, fill=tk.BOTH, padx=8, pady=4)

        # Sol Panel (2 Logo)
        self.left_logo_panel = tk.Frame(main_container, bg=self.COLOR_PANEL, width=150, bd=1, relief=tk.GROOVE)
        self.left_logo_panel.pack(side=tk.LEFT, fill=tk.Y, padx=(0, 4))
        self.left_logo_panel.pack_propagate(False)

        self.lbl_logo_l1 = tk.Label(self.left_logo_panel, bg=self.COLOR_PANEL)
        self.lbl_logo_l1.pack(side=tk.TOP, pady=15, expand=True)

        self.lbl_logo_l2 = tk.Label(self.left_logo_panel, bg=self.COLOR_PANEL)
        self.lbl_logo_l2.pack(side=tk.BOTTOM, pady=15, expand=True)

        # Sağ Panel (2 Logo)
        self.right_logo_panel = tk.Frame(main_container, bg=self.COLOR_PANEL, width=150, bd=1, relief=tk.GROOVE)
        self.right_logo_panel.pack(side=tk.RIGHT, fill=tk.Y, padx=(4, 0))
        self.right_logo_panel.pack_propagate(False)

        self.lbl_logo_r1 = tk.Label(self.right_logo_panel, bg=self.COLOR_PANEL)
        self.lbl_logo_r1.pack(side=tk.TOP, pady=15, expand=True)

        self.lbl_logo_r2 = tk.Label(self.right_logo_panel, bg=self.COLOR_PANEL)
        self.lbl_logo_r2.pack(side=tk.BOTTOM, pady=15, expand=True)

        # Orta Video Display
        display_frame = tk.Frame(main_container, bg=self.COLOR_DIM_GREEN, bd=2, relief=tk.GROOVE)
        display_frame.pack(side=tk.LEFT, expand=True, fill=tk.BOTH)

        self.canvas = tk.Canvas(
            display_frame, width=720, height=480,
            bg="#020403", highlightthickness=0
        )
        self.canvas.pack(expand=True, fill=tk.BOTH)

        self._draw_hud_overlay(720, 480, active=False)

        # ── TELEMETRY / BOTTOM STATUS BAR ────────────────────────────────────
        status_bar = tk.Frame(self.root, bg=self.COLOR_PANEL, bd=1, relief=tk.SOLID)
        status_bar.pack(fill=tk.X, side=tk.BOTTOM, padx=8, pady=(0, 8))

        self.status_var = tk.StringVar(value="SYS_INIT :: WAITING FOR TARGET SELECTION...")

        lbl_status = tk.Label(
            status_bar, textvariable=self.status_var,
            bg=self.COLOR_PANEL, fg=self.COLOR_HUD_GREEN,
            font=("Consolas", 9), anchor=tk.W, padx=8, pady=4
        )
        lbl_status.pack(side=tk.LEFT, fill=tk.X, expand=True)

        self.lbl_telemetry = tk.Label(
            status_bar, text="ALT: 420KM | LAT: 40.409 | LON: 49.867 | HDG: 180°",
            bg=self.COLOR_PANEL, fg=self.COLOR_TEXT_DIM,
            font=("Consolas", 9), anchor=tk.E, padx=8
        )
        self.lbl_telemetry.pack(side=tk.RIGHT)

    # ── LOGO YÜKLEME (DOSYA İSİMLERİ GÜNCELLENDİ) ─────────────────────────────

    def _load_logos(self):
        """Verilen gerçek görsel isimleriyle logoları sağ ve sola dağıtır."""
        logo_files = {
            # Sol Üst: Dinoco CubeSat Ekip Logosu
            "l1": ("WhatsApp_Image_2026-07-24_at_15.03.43_2_-removebg-preview.png", self.lbl_logo_l1),
            # Sol Alt: Mavi Yıldız ve Uydu Yörünge Logosu
            "l2": ("WhatsApp_Image_2026-07-24_at_15.03.43-removebg-preview.png", self.lbl_logo_l2),
            # Sağ Üst: AzST Beyaz Tipografi Logo
            "r1": ("WhatsApp_Image_2026-07-24_at_15.03.43_1_-removebg-preview.png", self.lbl_logo_r1),
            # Sağ Alt: Radar / Sinyal İkonu
            "r2": ("Gemini_Generated_Image_bfrcdibfrcdibfrc-removebg-preview.png", self.lbl_logo_r2)
        }

        for key, (filename, label_widget) in logo_files.items():
            if os.path.exists(filename):
                try:
                    img = Image.open(filename)
                    # Kaliteli yeniden boyutlandırma
                    img.thumbnail((150, 150), Image.Resampling.LANCZOS)
                    photo = ImageTk.PhotoImage(img)
                    self.logo_images[key] = photo
                    label_widget.config(image=photo)
                except Exception as e:
                    label_widget.config(text="[IMG ERR]", fg=self.COLOR_ALERT, font=("Consolas", 8))
            else:
                label_widget.config(
                    text=f"[FILE NOT FOUND]\n{filename[:15]}...",
                    fg=self.COLOR_ALERT, font=("Consolas", 7)
                    )
            #[AYXAN SALAM ]

    # ── HUD GRAPHICS ─────────────────────────────────────────────────────────

    def _draw_hud_overlay(self, w, h, active=True):
        cx, cy = w // 2, h // 2
        color = self.COLOR_HUD_GREEN if active else self.COLOR_TEXT_DIM

        gap = 20
        length = 30
        self.canvas.create_line(gap, gap, gap + length, gap, fill=color, width=2)
        self.canvas.create_line(gap, gap, gap, gap + length, fill=color, width=2)
        self.canvas.create_line(w - gap, gap, w - gap - length, gap, fill=color, width=2)
        self.canvas.create_line(w - gap, gap, w - gap, gap + length, fill=color, width=2)
        self.canvas.create_line(gap, h - gap, gap + length, h - gap, fill=color, width=2)
        self.canvas.create_line(gap, h - gap, gap, h - gap - length, fill=color, width=2)
        self.canvas.create_line(w - gap, h - gap, w - gap - length, h - gap, fill=color, width=2)
        self.canvas.create_line(w - gap, h - gap, w - gap, h - gap - length, fill=color, width=2)

        if not active:
            self.canvas.create_text(
                cx, cy,
                text="[ NO LIVE FEED // SELECT OPTICS & PRESS OSB 01 ]",
                fill=self.COLOR_AMBER, font=("Consolas", 12, "bold")
            )
            return

        r = 25
        self.canvas.create_oval(cx - r, cy - r, cx + r, cy + r, outline=color, width=1.5)
        self.canvas.create_line(cx - r - 15, cy, cx - r + 5, cy, fill=color, width=1.5)
        self.canvas.create_line(cx + r - 5, cy, cx + r + 15, cy, fill=color, width=1.5)
        self.canvas.create_line(cx, cy - r - 15, cx, cy - r + 5, fill=color, width=1.5)
        self.canvas.create_line(cx, cy + r - 5, cx, cy + r + 15, fill=color, width=1.5)

        self.canvas.create_line(cx - 80, cy - 60, cx - 40, cy - 60, fill=color, width=1)
        self.canvas.create_line(cx + 40, cy - 60, cx + 80, cy - 60, fill=color, width=1)
        self.canvas.create_line(cx - 80, cy + 60, cx - 40, cy + 60, fill=color, width=1)
        self.canvas.create_line(cx + 40, cy + 60, cx + 80, cy + 60, fill=color, width=1)

        self.canvas.create_text(cx - 95, cy - 60, text="+10", fill=color, font=("Consolas", 8))
        self.canvas.create_text(cx - 95, cy + 60, text="-10", fill=color, font=("Consolas", 8))

        self.canvas.create_text(40, cy, text="SPD\n240", fill=color, font=("Consolas", 10, "bold"))
        self.canvas.create_text(w - 40, cy, text="ALT\n420K", fill=color, font=("Consolas", 10, "bold"))

    # ── CAMERA SCAN ──────────────────────────────────────────────────────────

    def _scan_cameras(self):
        self.status_var.set("SYS_SCAN :: SCANNING OPTICAL INPUT SENSORS...")
        self.status_sys.config(text="SYS: SCANNING", fg=self.COLOR_AMBER)
        self.root.update()

        found = []
        for idx in range(8):
            cap = cv2.VideoCapture(idx, cv2.CAP_DSHOW if os.name == "nt" else cv2.CAP_ANY)
            if cap.isOpened():
                label = "PRIMARY CAM [INT]" if idx == 0 else f"PAYLOAD CAM [DEV_{idx}]"
                found.append(f"{idx} - {label}")
                cap.release()

        if not found:
            found = ["NO SENSOR DETECTED"]
            self.status_var.set("⚠️ SYS_ALERT :: NO PAYLOAD OPTICS DETECTED")
            self.status_sys.config(text="SYS: ERROR", fg=self.COLOR_ALERT)
        else:
            self.status_var.set(f"SYS_OK :: {len(found)} OPTICAL SENSORS ONLINE")
            self.status_sys.config(text="SYS: READY", fg=self.COLOR_HUD_GREEN)

        self.camera_combo["values"] = found
        self.camera_combo.current(min(1, len(found) - 1))

    # ── START / STOP ──────────────────────────────────────────────────────────

    def start_camera(self):
        entry = self.camera_var.get()
        if not entry or "NO SENSOR" in entry:
            messagebox.showwarning("TACTICAL ALERT", "No valid optical device selected!")
            return

        idx = int(entry.split()[0])
        backend = cv2.CAP_DSHOW if os.name == "nt" else cv2.CAP_ANY
        self.cap = cv2.VideoCapture(idx, backend)

        if not self.cap.isOpened():
            messagebox.showerror("SENSOR ERROR", f"Optics [{idx}] failed to initialize.")
            return

        self.cap.set(cv2.CAP_PROP_FRAME_WIDTH, 720)
        self.cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 540)

        self.running = True
        self.btn_start.config(state=tk.DISABLED)
        self.btn_stop.config(state=tk.NORMAL)
        self.btn_rec.config(state=tk.NORMAL)
        self.camera_combo.config(state=tk.DISABLED)

        self.status_var.set(f"LIVE_LINK :: OPTIC_{idx} TRANSMITTING ACTIVE SIGNAL")
        self.status_sys.config(text="SYS: TRANSMITTING", fg=self.COLOR_HUD_GREEN)

        self._update_frame()

    def stop_camera(self):
        if self.recording:
            self.stop_video_recording()

        self.running = False
        if self.cap:
            self.cap.release()
            self.cap = None

        self.btn_start.config(state=tk.NORMAL)
        self.btn_stop.config(state=tk.DISABLED)
        self.btn_rec.config(state=tk.DISABLED)
        self.camera_combo.config(state="readonly")

        self.status_var.set("SYS_STANDBY :: FEED TERMINATED BY OPERATOR")
        self.status_sys.config(text="SYS: STANDBY", fg=self.COLOR_AMBER)
        self.fps_label.config(text="FPS: 00")

        self.canvas.delete("all")
        self._draw_hud_overlay(self.canvas.winfo_width(), self.canvas.winfo_height(), active=False)

    # ── RECORDING & SNAPSHOT COMBINED ────────────────────────────────────────

    def toggle_record_and_snap(self):
        if self.current_frame is None:
            return

        timestamp = int(time.time())

        # 1. Anlık Ekran Görüntüsü Kaydı
        snap_filename = f"CubeSat_SNAP_{timestamp}.png"
        cv2.imwrite(snap_filename, self.current_frame)

        # 2. Video Kaydı Kontrolü (Start / Stop)
        if not self.recording:
            video_filename = f"CubeSat_REC_{timestamp}.mp4"
            h, w = self.current_frame.shape[:2]
            fourcc = cv2.VideoWriter_fourcc(*'mp4v')
            self.video_writer = cv2.VideoWriter(video_filename, fourcc, 20.0, (w, h))
            self.recording = True

            self.rec_indicator.config(text="● REC")
            self.btn_rec.config(text="[ OSB 03: STOP REC ]", fg=self.COLOR_ALERT)
            self.status_var.set(f"RECORDING STARTED :: {video_filename} & SNAPSHOT SAVED")
        else:
            self.stop_video_recording()
            self.status_var.set(f"RECORDING STOPPED & SNAPSHOT SAVED: {snap_filename}")

    def stop_video_recording(self):
        if self.recording:
            self.recording = False
            if self.video_writer:
                self.video_writer.release()
                self.video_writer = None
            self.rec_indicator.config(text="")
            self.btn_rec.config(text="[ OSB 03: REC / SNAP ]", fg=self.COLOR_AMBER)

    # ── FRAME LOOP ────────────────────────────────────────────────────────────

    def _update_frame(self):
        if not self.running:
            return

        ret, frame = self.cap.read()
        if ret:
            self.current_frame = frame.copy()

            if self.recording and self.video_writer:
                self.video_writer.write(self.current_frame)

            canvas_w = self.canvas.winfo_width() or 720
            canvas_h = self.canvas.winfo_height() or 480

            h, w = frame.shape[:2]
            scale = min(canvas_w / w, canvas_h / h)
            nw, nh = int(w * scale), int(h * scale)
            frame_resized = cv2.resize(frame, (nw, nh))

            frame_rgb = cv2.cvtColor(frame_resized, cv2.COLOR_BGR2RGB)
            img = ImageTk.PhotoImage(Image.fromarray(frame_rgb))

            self.canvas.delete("all")
            self.canvas.create_image(canvas_w // 2, canvas_h // 2, anchor=tk.CENTER, image=img)
            self.canvas._img = img

            self._draw_hud_overlay(canvas_w, canvas_h, active=True)

            self.fps_counter += 1
            now = time.time()
            if now - self.fps_time >= 1.0:
                self.fps_display = self.fps_counter
                self.fps_counter = 0
                self.fps_time = now
                self.fps_label.config(text=f"FPS: {self.fps_display:02d}")

                hdg = random.randint(178, 182)
                self.lbl_telemetry.config(text=f"ALT: 420KM | LAT: 40.409 | LON: 49.867 | HDG: {hdg}°")
        else:
            self.status_var.set("⚠️ SIGNAL_LOSS :: NO FRAME RECEIVED FROM SENSOR")
            self.status_sys.config(text="SYS: NO SIGNAL", fg=self.COLOR_ALERT)

        self.root.after(30, self._update_frame)

    # ── WINDOW CLOSE ─────────────────────────────────────────────────────────

    def on_closing(self):
        self.stop_camera()
        self.root.destroy()


def main():
    root = tk.Tk()
    app = F16CameraStation(root)
    root.protocol("WM_DELETE_WINDOW", app.on_closing)
    root.mainloop()


if __name__ == "__main__":
    main()