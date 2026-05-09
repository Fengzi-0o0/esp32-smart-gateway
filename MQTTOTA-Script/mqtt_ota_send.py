import paho.mqtt.client as mqtt
import struct
import time
import ssl
import os
import json
import sys
import threading
import tkinter as tk
from tkinter import filedialog, messagebox, font as tkfont

CHUNK_SIZE = 1024


class OtaSender:
    C_BG      = "#f3f3f3"
    C_CARD    = "#ffffff"
    C_ACC     = "#0078d4"
    C_ACC_H   = "#106ebe"
    C_W       = "#0d0d0d"
    C_M       = "#3d3d3d"
    C_D       = "#f0f0f0"
    C_BORDER  = "#d4d4d4"
    C_LOG_BG  = "#fafafa"

    def _config_path(self):
        if getattr(sys, 'frozen', False):
            base = os.path.dirname(sys.executable)
        else:
            base = os.path.dirname(os.path.abspath(__file__))
        return os.path.join(base, "ota_config.json")

    L = {
        "en": {
            "subtitle":     "Wireless Firmware Update",
            "conn":         "CONNECTION",
            "broker":       "Broker",
            "port":         "Port",
            "user":         "Username",
            "pass":         "Password",
            "topic":        "Topic",
            "target":       "Target Device ID",
            "fw":           "FIRMWARE",
            "browse":       "Browse",
            "ssl":          "SSL / TLS",
            "start":        "Start OTA Upload",
            "sending":      "Sending...",
            "prog":         "PROGRESS",
            "ready":        "Ready",
            "log":          "LOG",
            "done":         "\u2713  Upload Complete \u2014 Device is restarting",
            "sel_fw":       "Select firmware",
            "bin":          "Binary",
            "all":          "All",
            "err_broker":   "Please enter Broker address",
            "err_port":     "Please enter Port",
            "err_topic":    "Please enter Topic",
            "err_file":     "Please select firmware file",
            "err_notfound": "File not found:\n",
            "err_bin":      "Only .bin files supported",
            "lang":         "EN",
        },
        "zh": {
            "subtitle":     "无线固件升级",
            "conn":         "连接配置",
            "broker":       "代理地址",
            "port":         "端口",
            "user":         "用户名",
            "pass":         "密码",
            "topic":        "主题",
            "target":       "目标设备 ID（可指定，空则是全部）",
            "fw":           "固件配置",
            "browse":       "浏览",
            "ssl":          "SSL / TLS",
            "start":        "开始 OTA 上传",
            "sending":      "上传中...",
            "prog":         "传输进度",
            "ready":        "就绪",
            "log":          "日志",
            "done":         "\u2713  上传完成 \u2014 设备正在重启",
            "sel_fw":       "选择固件文件",
            "bin":          "二进制文件",
            "all":          "所有文件",
            "err_broker":   "请输入代理地址",
            "err_port":     "请输入端口",
            "err_topic":    "请输入主题",
            "err_file":     "请选择固件文件",
            "err_notfound": "文件未找到：\n",
            "err_bin":      "仅支持 .bin 文件",
            "lang":         "中文",
        },
    }

    def __init__(self, root):
        self.root = root
        self.root.title("ESP32  ·  MQTT OTA")
        self.root.configure(bg=self.C_BG)
        self.root.resizable(False, False)
        self.root.geometry("900x640")

        self.sending = False
        self._prog = 0
        self.lang = "zh"
        self._init_font()
        self.build_ui()
        self._load_config()
        self.root.protocol("WM_DELETE_WINDOW", self._on_close)

    def _save_config(self):
        data = {
            "broker": self.e_broker.get().strip(),
            "port":   self.e_port.get().strip(),
            "user":   self.e_user.get().strip(),
            "pass":   self.e_pass.get().strip(),
            "topic":  self.e_topic.get().strip(),
            "target": self.e_target.get().strip(),
            "file":   self.e_file.get().strip(),
            "ssl":    self.ssl_var.get(),
            "lang":   self.lang,
        }
        try:
            with open(self._config_path(), "w", encoding="utf-8") as f:
                json.dump(data, f, ensure_ascii=False, indent=2)
        except Exception:
            pass

    def _load_config(self):
        path = self._config_path()
        if not os.path.exists(path):
            return
        try:
            with open(path, "r", encoding="utf-8") as f:
                data = json.load(f)
        except Exception:
            return

        fields = {
            "broker": self.e_broker,
            "port":   self.e_port,
            "user":   self.e_user,
            "pass":   self.e_pass,
            "topic":  self.e_topic,
            "target": self.e_target,
            "file":   self.e_file,
        }
        for key, entry in fields.items():
            val = data.get(key, "")
            if val:
                entry.delete(0, "end")
                entry.insert(0, val)

        saved_ssl = data.get("ssl", True)
        self.ssl_var.set(saved_ssl)

        saved_lang = data.get("lang", "zh")
        if saved_lang in self.L:
            self.lang = saved_lang
            self._refresh_lang()

    def _on_close(self):
        self._save_config()
        self.root.destroy()

    def _toggle_lang(self):
        self.lang = "zh" if self.lang == "en" else "en"
        self._refresh_lang()

    def t(self, key):
        return self.L[self.lang][key]

    def _init_font(self):
        self.fn = {
            "h":  tkfont.Font(family="Segoe UI", size=22, weight="bold"),
            "s":  tkfont.Font(family="Segoe UI", size=10),
            "m":  tkfont.Font(family="Segoe UI", size=9),
            "b":  tkfont.Font(family="Segoe UI", size=11, weight="bold"),
            "l":  tkfont.Font(family="Consolas",  size=9),
        }

    # ═══════════════════════════════════════════════════════════
    #  主布局
    # ═══════════════════════════════════════════════════════════
    def build_ui(self):
        self.root.columnconfigure(0, weight=0, minsize=300)
        self.root.columnconfigure(1, weight=1)
        self.root.rowconfigure(0, weight=1)

        left = tk.Frame(self.root, bg=self.C_BG, padx=16, pady=12)
        left.grid(row=0, column=0, sticky="ns")

        right = tk.Frame(self.root, bg=self.C_BG, padx=16, pady=12)
        right.grid(row=0, column=1, sticky="nsew")

        self._build_left(left)
        self._build_right(right)

    # ═══════════════════════════════════════════════════════════
    #  左栏
    # ═══════════════════════════════════════════════════════════
    def _build_left(self, parent):
        parent.columnconfigure(0, weight=1)

        r = 0

        hdr = tk.Frame(parent, bg=self.C_BG)
        hdr.grid(row=r, column=0, sticky="ew")
        r += 1

        tk.Label(hdr, text="MQTT OTA", font=self.fn["h"],
                 bg=self.C_BG, fg=self.C_W).pack(side="left")

        self.btn_lang = tk.Button(
            hdr, text=self.t("lang"), command=self._toggle_lang,
            font=self.fn["m"], bg=self.C_D, fg=self.C_W,
            relief="flat", bd=0, padx=10, pady=4,
            cursor="hand2", activebackground=self.C_BORDER,
            highlightthickness=1,
            highlightbackground=self.C_BORDER)
        self.btn_lang.pack(side="right", padx=(8, 0))

        self.lbl_sub = tk.Label(parent, text=self.t("subtitle"),
                                font=self.fn["s"], bg=self.C_BG,
                                fg=self.C_M, anchor="w")
        self.lbl_sub.grid(row=r, column=0, sticky="w", pady=(0, 8))
        r += 1

        cc = tk.Frame(parent, bg=self.C_CARD, padx=12, pady=8,
                      highlightthickness=1,
                      highlightbackground=self.C_BORDER)
        cc.grid(row=r, column=0, sticky="ew", pady=(0, 6))
        r += 1

        self.lbl_conn = tk.Label(cc, text=self.t("conn"),
                                 font=self.fn["m"], bg=self.C_CARD,
                                 fg=self.C_M, anchor="w")
        self.lbl_conn.pack(fill="x", pady=(0, 4))

        self.e_broker = self._fld(cc, "broker",
                                   "xxxxxxx.xxxxx.xxx-xxxxxx.emqxsl.cn")
        self.e_port   = self._fld(cc, "port",     "8883")
        self.e_user   = self._fld(cc, "user", "Userota")
        self.e_pass   = self._fld(cc, "pass", "12345678",
                                   show="\u2022")
        self.e_topic  = self._fld(cc, "topic",    "esp32/iosetting/ota")
        self.e_target = self._fld(cc, "target", "")

        fc = tk.Frame(parent, bg=self.C_CARD, padx=12, pady=8,
                      highlightthickness=1,
                      highlightbackground=self.C_BORDER)
        fc.grid(row=r, column=0, sticky="ew", pady=(0, 6))
        r += 1

        self.lbl_fw = tk.Label(fc, text=self.t("fw"),
                               font=self.fn["m"], bg=self.C_CARD,
                               fg=self.C_M, anchor="w")
        self.lbl_fw.pack(fill="x", pady=(0, 4))

        row = tk.Frame(fc, bg=self.C_CARD)
        row.pack(fill="x", pady=(0, 4))

        self.e_file = tk.Entry(
            row, font=self.fn["s"], bg=self.C_D, fg=self.C_W,
            insertbackground=self.C_W, relief="flat", bd=0,
            highlightthickness=1,
            highlightbackground=self.C_BORDER,
            highlightcolor=self.C_ACC)
        self.e_file.pack(side="left", fill="x", expand=True,
                         ipady=5, ipadx=6)

        self.btn_browse = tk.Button(
            row, text=self.t("browse"), command=self._browse,
            font=self.fn["s"], bg=self.C_CARD, fg=self.C_W,
            relief="flat", bd=0, padx=10, pady=5,
            cursor="hand2", activebackground=self.C_D,
            highlightthickness=1,
            highlightbackground=self.C_BORDER)
        self.btn_browse.pack(side="right", padx=(6, 0))

        sr = tk.Frame(fc, bg=self.C_CARD)
        sr.pack(fill="x")

        self.lbl_ssl = tk.Label(sr, text=self.t("ssl"),
                                font=self.fn["s"], bg=self.C_CARD,
                                fg=self.C_W)
        self.lbl_ssl.pack(side="left")

        self.ssl_var = tk.BooleanVar(value=True)
        self._sw(sr, self.ssl_var)

        self.btn = tk.Button(
            parent, text=self.t("start"),
            command=self._go, font=self.fn["b"],
            bg=self.C_ACC, fg="#ffffff",
            relief="flat", bd=0, pady=10,
            cursor="hand2",
            activebackground=self.C_ACC_H,
            activeforeground="#ffffff")
        self.btn.grid(row=r, column=0, sticky="ew", pady=(4, 0))

    # ═══════════════════════════════════════════════════════════
    #  右栏
    # ═══════════════════════════════════════════════════════════
    def _build_right(self, parent):
        parent.columnconfigure(0, weight=1)
        parent.rowconfigure(0, weight=0)
        parent.rowconfigure(1, weight=1)

        pc = tk.Frame(parent, bg=self.C_CARD, padx=12, pady=8,
                      highlightthickness=1,
                      highlightbackground=self.C_BORDER)
        pc.grid(row=0, column=0, sticky="ew", pady=(0, 6))

        self.lbl_prog = tk.Label(pc, text=self.t("prog"),
                                 font=self.fn["m"], bg=self.C_CARD,
                                 fg=self.C_M, anchor="w")
        self.lbl_prog.pack(fill="x", pady=(0, 4))

        self._pc = tk.Canvas(pc, height=6,
                             bg=self.C_D, highlightthickness=0, bd=0)
        self._pc.pack(fill="x", pady=(0, 4))

        self._pl = tk.Label(pc, text=self.t("ready"),
                            font=self.fn["s"],
                            bg=self.C_CARD, fg=self.C_M)
        self._pl.pack(anchor="w")

        self._df = tk.Frame(pc, bg=self.C_CARD)
        self._dl = tk.Label(self._df, font=self.fn["s"],
                            bg=self.C_CARD, fg="#107c10")

        self._pc.bind("<Configure>", lambda e: self._draw_bg())
        self._draw_bg()

        lc = tk.Frame(parent, bg=self.C_CARD, padx=12, pady=8,
                      highlightthickness=1,
                      highlightbackground=self.C_BORDER)
        lc.grid(row=1, column=0, sticky="nsew")
        lc.columnconfigure(0, weight=1)
        lc.rowconfigure(1, weight=1)

        self.lbl_log = tk.Label(lc, text=self.t("log"),
                                font=self.fn["m"], bg=self.C_CARD,
                                fg=self.C_M, anchor="w")
        self.lbl_log.grid(row=0, column=0, sticky="ew", pady=(0, 4))

        box = tk.Frame(lc, bg=self.C_LOG_BG)
        box.grid(row=1, column=0, sticky="nsew")
        box.columnconfigure(0, weight=1)
        box.rowconfigure(0, weight=1)

        self.log = tk.Text(
            box, bg=self.C_LOG_BG, fg=self.C_W,
            font=self.fn["l"], wrap="word",
            bd=0, padx=8, pady=6,
            insertbackground=self.C_W,
            selectbackground=self.C_ACC,
            selectforeground="#ffffff",
            relief="flat")
        self.log.grid(row=0, column=0, sticky="nsew")

        sb = tk.Scrollbar(box, command=self.log.yview, width=10)
        sb.grid(row=0, column=1, sticky="ns")
        self.log.configure(yscrollcommand=sb.set)
        try:
            sb.configure(bg=self.C_LOG_BG, troughcolor=self.C_LOG_BG,
                         relief="flat", highlightthickness=0,
                         activebackground=self.C_BORDER)
        except Exception:
            pass

    def _fld(self, p, key, default, show=None):
        lbl = tk.Label(p, text=self.t(key), font=self.fn["m"],
                       bg=self.C_CARD, fg=self.C_M,
                       anchor="w")
        lbl.pack(fill="x", pady=(2, 0))
        if not hasattr(self, "_fld_labels"):
            self._fld_labels = {}
        self._fld_labels[key] = lbl

        e = tk.Entry(p, font=self.fn["s"], show=show,
                     bg=self.C_D, fg=self.C_W,
                     insertbackground=self.C_W,
                     relief="flat", bd=0,
                     highlightthickness=1,
                     highlightbackground=self.C_BORDER,
                     highlightcolor=self.C_ACC)
        e.insert(0, default)
        e.pack(fill="x", ipady=5, ipadx=6, pady=(0, 2))
        return e

    def _sw(self, p, var):
        W, H = 36, 20
        kn = H - 6

        cv = tk.Canvas(p, width=W, height=H,
                       bg=self.C_CARD, highlightthickness=0, bd=0)
        cv.pack(side="right")

        def rr(x1, y1, x2, y2, r, **kw):
            r = min(r, (x2 - x1) // 2, (y2 - y1) // 2)
            if r < 1:
                return cv.create_rectangle(x1, y1, x2, y2, **kw)
            pts = [x1+r, y1, x2-r, y1, x2, y1, x2, y1+r,
                   x2, y2-r, x2, y2, x2-r, y2, x1+r, y2,
                   x1, y2, x1, y2-r, x1, y1+r, x1, y1]
            return cv.create_polygon(pts, smooth=True, **kw)

        def draw():
            cv.delete("all")
            if var.get():
                rr(0, 0, W, H, H // 2, fill=self.C_ACC, outline="")
                cv.create_oval(W - kn - 2, 3, W - 3, kn + 3,
                               fill="#ffffff", outline="")
            else:
                rr(0, 0, W, H, H // 2, fill=self.C_BORDER, outline="")
                cv.create_oval(2, 3, kn + 2, kn + 3,
                               fill="#ffffff", outline="")

        cv.bind("<Button-1>", lambda e: (var.set(not var.get()), draw()))
        draw()

    def _draw_bg(self):
        self._pc.delete("bg")
        w = self._pc.winfo_width() or 1
        h = self._pc.winfo_height() or 6
        self._pc.create_rectangle(
            0, 0, w, h, fill=self.C_D, outline="", tags="bg")
        self._fill(self._prog)

    def _fill(self, pct):
        self._prog = pct
        self._pc.delete("bar")
        w = self._pc.winfo_width() or 1
        h = self._pc.winfo_height() or 6
        if pct > 0:
            self._pc.create_rectangle(
                0, 0, max(1, w * pct / 100), h,
                fill=self.C_ACC, outline="", tags="bar")

    def _log(self, msg):
        self.log.insert("end", msg + "\n")
        self.log.see("end")

    def _sp(self, pct, txt):
        self.root.after(0, lambda: self.__sp(pct, txt))

    def __sp(self, pct, txt):
        self._fill(pct)
        self._pl.configure(text="{}%  \u2014  {}".format(pct, txt))

    def _show_done(self):
        self._df.pack(fill="x", pady=(4, 0))
        self._dl.configure(
            text=self.t("done"),
            fg="#ffffff", bg="#107c10", padx=10, pady=6)
        self._dl.pack(fill="x")

    def _browse(self):
        p = filedialog.askopenfilename(
            title=self.t("sel_fw"),
            filetypes=[
                (self.t("bin"), "*.bin"),
                (self.t("all"), "*.*")])
        if p:
            self.e_file.delete(0, "end")
            self.e_file.insert(0, p)

    def _refresh_lang(self):
        self.btn_lang.configure(text=self.t("lang"))
        self.lbl_sub.configure(text=self.t("subtitle"))
        self.lbl_conn.configure(text=self.t("conn"))
        self.lbl_fw.configure(text=self.t("fw"))
        self.lbl_prog.configure(text=self.t("prog"))
        self.lbl_log.configure(text=self.t("log"))
        self.lbl_ssl.configure(text=self.t("ssl"))

        for key, lbl in self._fld_labels.items():
            lbl.configure(text=self.t(key))

        self.btn_browse.configure(text=self.t("browse"))

        if not self.sending:
            self.btn.configure(text=self.t("start"))
            self._pl.configure(text=self.t("ready"))

    def _go(self):
        if self.sending:
            return

        broker = self.e_broker.get().strip()
        port   = self.e_port.get().strip()
        user   = self.e_user.get().strip()
        pwd    = self.e_pass.get().strip()
        topic  = self.e_topic.get().strip()
        target = self.e_target.get().strip()
        fp     = self.e_file.get().strip()

        if not broker:
            messagebox.showerror("Error", self.t("err_broker"))
            return
        if not port:
            messagebox.showerror("Error", self.t("err_port"))
            return
        if not topic:
            messagebox.showerror("Error", self.t("err_topic"))
            return
        if not fp:
            messagebox.showerror("Error", self.t("err_file"))
            return
        if not os.path.exists(fp):
            messagebox.showerror("Error", self.t("err_notfound") + fp)
            return
        if not fp.endswith(".bin"):
            messagebox.showerror("Error", self.t("err_bin"))
            return

        self.sending = True
        self._df.pack_forget()
        self.btn.configure(state="disabled", bg=self.C_BORDER,
                           fg=self.C_M, text=self.t("sending"))
        self.log.delete("1.0", "end")
        self.__sp(0, "Starting...")

        threading.Thread(
            target=self._run,
            args=(broker, port, user, pwd, topic,
                  fp, target, self.ssl_var.get()),
            daemon=True).start()

    def _run(self, broker, port, user, pwd, topic,
             fp, target, ssl_on):
        try:
            self._send(broker, port, user, pwd, topic,
                       fp, target, ssl_on)
        except Exception as _e:
            self._log("[ERROR] " + str(_e))
            self._sp(0, "Error")
        finally:
            self.sending = False
            self.root.after(0, lambda: self.btn.configure(
                state="normal", bg=self.C_ACC, fg="#ffffff",
                text=self.t("start")))

    def _send(self, broker, port, user, pwd, topic,
              fp, target, ssl_on):
        size = os.path.getsize(fp)
        total = (size + CHUNK_SIZE - 1) // CHUNK_SIZE

        self._log("[INFO] File: {}".format(os.path.basename(fp)))
        self._log("[INFO] Size: {} B  ({} KB)".format(size, size // 1024))
        self._log("[INFO] Chunks: {} \u00d7 {} B".format(total, CHUNK_SIZE))
        if target:
            self._log("[INFO] Target: " + target)
        else:
            self._log("[INFO] Target: ALL devices on " + topic)

        st = topic + "/status"
        ud = {"st": st, "sender": self}

        c = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, userdata=ud)
        c.on_connect = self._cb_conn
        c.on_message = self._cb_msg
        c.username_pw_set(user, pwd)

        if ssl_on:
            c.tls_set(cert_reqs=ssl.CERT_NONE)
            c.tls_insecure_set(True)

        self._log("[INFO] Connecting to {}:{} ...".format(broker, port))
        self._sp(0, "Connecting...")

        c.connect(broker, int(port), 60)
        c.loop_start()
        time.sleep(2)

        self._log("[OTA] [1/3] Sending START...")
        self._sp(0, "Sending START...")
        msg = {"cmd": "START", "size": size, "chunks": total}
        if target:
            msg["target"] = target
        c.publish(topic, json.dumps(msg), qos=1)
        time.sleep(2)

        self._log("[OTA] [2/3] Sending firmware...")
        self._sp(0, "Sending...")

        sent = 0
        idx = 0
        with open(fp, "rb") as f:
            while True:
                d = f.read(CHUNK_SIZE)
                if not d:
                    break
                hdr = struct.pack("<BI", 0x01, idx)
                c.publish(topic, hdr + d, qos=0)
                sent += len(d)
                idx += 1
                if idx % 8 == 0:
                    p = int(sent / size * 100)
                    self._log("[OTA] {}%  ({} / {} KB)".format(
                        p, sent // 1024, size // 1024))
                    self._sp(p, "{} / {} KB".format(
                        sent // 1024, size // 1024))
                time.sleep(0.05)

        self._log("[OTA] Sent: {} chunks, {} bytes".format(idx, sent))
        self._sp(99, "Finalizing...")

        self._log("[OTA] [3/3] Sending DONE...")
        c.publish(topic, "DONE", qos=1)
        time.sleep(10)

        c.loop_stop()
        c.disconnect()
        self._log("[INFO] Complete!")
        self._sp(100, "Done!")
        self.root.after(0, self._show_done)

    @staticmethod
    def _cb_conn(client, userdata, flags, rc, props):
        s = userdata["sender"]
        if rc == 0:
            client.subscribe(userdata["st"], qos=1)
            s.root.after(0, lambda: s._log("[MQTT] Connected"))
        else:
            s.root.after(0, lambda: s._log(
                "[MQTT] Failed: " + str(rc)))

    @staticmethod
    def _cb_msg(client, userdata, msg):
        s = userdata["sender"]
        try:
            d = json.loads(msg.payload.decode())
            st  = d.get("status", "")
            pct = d.get("progress", 0)
            ck  = d.get("chunk", 0)
            wr  = d.get("written", 0)

            m = {
                "started":    "[Device] OTA started",
                "receiving":  "[Device] {}% (chunk {}, {} KB)".format(
                                  pct, ck, wr // 1024),
                "finalizing": "[Device] Finalizing...",
                "success":    "[Device] SUCCESS! Restarting...",
                "error":      "[Device] ERROR (chunk {})".format(ck),
                "timeout":    "[Device] Timeout",
                "aborted":    "[Device] Aborted",
            }.get(st, "[Device] " + str(d))

            s.root.after(0, lambda: s._dev(m, pct, st))
        except Exception:
            pass

    def _dev(self, line, pct, status):
        self._log(line)
        if status == "receiving":
            self._sp(pct, "Device: {}%".format(pct))
        elif status == "success":
            self._sp(100, "SUCCESS! Restarting...")
            self._show_done()
        elif status == "error":
            self._sp(0, "Device ERROR")


def main():
    root = tk.Tk()
    OtaSender(root)
    root.mainloop()


if __name__ == "__main__":
    main()
##pyinstaller --onefile --windowed --name "MQTT_OTA" C:\Users\86159\Desktop\esp32物联网集群开发\mqtt_ota_send.py
